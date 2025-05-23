//
//  KRCamera.cpp
//  Kraken Engine
//
//  Copyright 2024 Kearwood Gilbert. All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification, are
//  permitted provided that the following conditions are met:
//  
//  1. Redistributions of source code must retain the above copyright notice, this list of
//  conditions and the following disclaimer.
//  
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//  of conditions and the following disclaimer in the documentation and/or other materials
//  provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY KEARWOOD GILBERT ''AS IS'' AND ANY EXPRESS OR IMPLIED
//  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KEARWOOD GILBERT OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  
//  The views and conclusions contained in the software and documentation are those of the
//  authors and should not be interpreted as representing official policies, either expressed
//  or implied, of Kearwood Gilbert.
//

#include "KREngine-common.h"
#include "KRCamera.h"
#include "KRDirectionalLight.h"
#include "KRRenderPass.h"
#include "KRPipeline.h"
#include "KRRenderPass.h"
#include "KRRenderGraph.h"

using namespace mimir;
using namespace hydra;

/* static */
void KRCamera::InitNodeInfo(KrNodeInfo* nodeInfo)
{
  KRNode::InitNodeInfo(nodeInfo);
  nodeInfo->camera.surfaceHandle = 1;
  nodeInfo->camera.skybox_texture = KR_NULL_HANDLE;
}

KrResult KRCamera::update(const KrNodeInfo* nodeInfo)
{
  KrResult res = KRNode::update(nodeInfo);
  if (res != KR_SUCCESS) {
    return res;
  }
  m_surfaceHandle = nodeInfo->camera.surfaceHandle;

  KRTexture* skybox_texture = nullptr;
  if (nodeInfo->camera.skybox_texture != KR_NULL_HANDLE) {
    res = m_pContext->getMappedResource<KRTexture>(nodeInfo->camera.skybox_texture, &skybox_texture);
    if (res != KR_SUCCESS) {
      return res;
    }
  }
  m_pSkyBoxTexture = skybox_texture;
  if (m_pSkyBoxTexture) {
    m_skyBox = m_pSkyBoxTexture->getName();
  } else {
    m_skyBox = "";
  }

  return KR_SUCCESS;
}

KRCamera::KRCamera(KRScene& scene, std::string name)
  : KRNode(scene, name)
{
  m_surfaceHandle = KR_NULL_HANDLE;
  m_last_frame_start = 0;

  m_particlesAbsoluteTime = 0.0f;
  volumetricBufferWidth = 0;
  volumetricBufferHeight = 0;
  m_pSkyBoxTexture = NULL;

  compositeDepthTexture = 0;
  compositeColorTexture = 0;
  lightAccumulationTexture = 0;
  compositeFramebuffer = 0;
  lightAccumulationBuffer = 0;

  volumetricLightAccumulationBuffer = 0;
  volumetricLightAccumulationTexture = 0;
  m_frame_times_filled = 0;

  m_fade_color = Vector4::Zero();

  m_debug_text_vbo_data.init(m_pContext->getMeshManager(), &m_debug_text_vertices, nullptr, (1 << KRMesh::KRENGINE_ATTRIB_VERTEX) | (1 << KRMesh::KRENGINE_ATTRIB_TEXUVA), true, KRMeshManager::KRVBOData::IMMEDIATE
#if KRENGINE_DEBUG_GPU_LABELS
    , "Debug Text"
#endif
  );
}

KRCamera::~KRCamera()
{
  destroyBuffers();
}

std::string KRCamera::getElementName()
{
  return "camera";
}

tinyxml2::XMLElement* KRCamera::saveXML(tinyxml2::XMLNode* parent)
{
  tinyxml2::XMLElement* e = KRNode::saveXML(parent);
  e->SetAttribute("skybox", m_skyBox.c_str());

  return e;
}


void KRCamera::loadXML(tinyxml2::XMLElement* e)
{
  KRNode::loadXML(e);
  const char* szSkyBoxName = e->Attribute("skybox");
  m_skyBox = szSkyBoxName ? szSkyBoxName : "";

  unsigned int surfaceHandle = 1;
  if (e->QueryUnsignedAttribute("surface", &surfaceHandle) != tinyxml2::XML_SUCCESS) {
    surfaceHandle = 1;
  }
  m_surfaceHandle = surfaceHandle;
}

void KRCamera::setSkyBox(const std::string& skyBox)
{
  m_pSkyBoxTexture = NULL;
  m_skyBox = skyBox;
}

const std::string KRCamera::getSkyBox() const
{
  return m_skyBox;
}

void KRCamera::render(KRNode::RenderInfo& ri)
{
  KRScene& scene = getScene();

  switch (ri.renderPass->getType()) {
  case RenderPassType::RENDER_PASS_FORWARD_OPAQUE:
    {
      // ----====---- Sky Box ----====----

      GL_PUSH_GROUP_MARKER("Sky Box");

      if (!m_pSkyBoxTexture && m_skyBox.length()) {
        m_pSkyBoxTexture = getContext().getTextureManager()->getTextureCube(m_skyBox.c_str());
      }

      if (m_pSkyBoxTexture) {
        m_pSkyBoxTexture->resetPoolExpiry(0.0f, KRTexture::TEXTURE_USAGE_SKY_CUBE);

        std::string shader_name("sky_box");
        PipelineInfo info{};
        info.shader_name = &shader_name;
        info.pCamera = this;
        info.renderPass = ri.renderPass;
        info.rasterMode = RasterMode::kOpaqueNoDepthWrite;
        info.cullMode = CullMode::kCullNone;

        KRPipeline* pPipeline = getContext().getPipelineManager()->getPipeline(*ri.surface, info);
        pPipeline->setImageBinding("diffuseTexture", m_pSkyBoxTexture, getContext().getSamplerManager()->DEFAULT_CLAMPED_SAMPLER);
        pPipeline->bind(ri, Matrix4());

        // Render a full screen quad
        m_pContext->getMeshManager()->bindVBO(ri.commandBuffer, &getContext().getMeshManager()->KRENGINE_VBO_DATA_2D_SQUARE_VERTICES, 1.0f);
        vkCmdDraw(ri.commandBuffer, 4, 1, 0, 0);
      }

      GL_POP_GROUP_MARKER;
    }
    break;
   case RenderPassType::RENDER_PASS_DEBUG_OVERLAYS:
    {
      // ----====---- Debug Overlays ----====----

      if (settings.debug_display == KRRenderSettings::KRENGINE_DEBUG_DISPLAY_OCTREE) {
        KRMeshManager::KRVBOData& vertices = getContext().getMeshManager()->KRENGINE_VBO_DATA_3D_CUBE_VERTICES;

        PipelineInfo info{};
        std::string shader_name("visualize_overlay");
        info.shader_name = &shader_name;
        info.pCamera = this;
        info.renderPass = ri.renderPass;
        info.rasterMode = RasterMode::kAdditive;
        info.vertexAttributes = vertices.getVertexAttributes();
        info.modelFormat = ModelFormat::KRENGINE_MODEL_FORMAT_STRIP;
        KRPipeline* pVisShader = getContext().getPipelineManager()->getPipeline(*ri.surface, info);


        m_pContext->getMeshManager()->bindVBO(ri.commandBuffer, &vertices, 1.0f);
        for (unordered_map<AABB, int>::iterator itr = m_viewport.getVisibleBounds().begin(); itr != m_viewport.getVisibleBounds().end(); itr++) {
          Matrix4 matModel = Matrix4();
          matModel.scale((*itr).first.size() * 0.5f);
          matModel.translate((*itr).first.center());
          pVisShader->bind(ri, matModel);
          vkCmdDraw(ri.commandBuffer, 14, 1, 0, 0);
        }
      }

      renderDebug(ri);
    }
    break;
   case RenderPassType::RENDER_PASS_POST_COMPOSITE:
     {
      renderPost(ri);
     }
     break;
  default:
    break;
  }
  
  scene.render(ri);

  switch (ri.renderPass->getType()) {
  default:
    break;
  }
}

void KRCamera::renderFrame(VkCommandBuffer& commandBuffer, KRSurface& compositeSurface, KRRenderGraph& renderGraph)
{
  if (compositeSurface.m_handle != m_surfaceHandle) {
    // Only render to the assigned surface
    return;
  }

  // ----====---- Record timing information for measuring FPS ----====----
  uint64_t current_time = m_pContext->getAbsoluteTimeMilliseconds();
  if (m_last_frame_start != 0) {
    m_frame_times[m_pContext->getCurrentFrame() % KRAKEN_FPS_AVERAGE_FRAME_COUNT] = (int)(current_time - m_last_frame_start);
    if (m_frame_times_filled < KRAKEN_FPS_AVERAGE_FRAME_COUNT) m_frame_times_filled++;
  }
  m_last_frame_start = current_time;

  createBuffers(compositeSurface.getWidth(), compositeSurface.getHeight());

  KRScene& scene = getScene();

  Matrix4 modelMatrix = getModelMatrix();
  Matrix4 viewMatrix = Matrix4::LookAt(Matrix4::Dot(modelMatrix, Vector3::Zero()), Matrix4::Dot(modelMatrix, Vector3::Forward()), Vector3::Normalize(Matrix4::DotNoTranslate(modelMatrix, Vector3::Up())));

  //Matrix4 viewMatrix = Matrix4::Invert(getModelMatrix());

  settings.setViewportSize(Vector2::Create((float)compositeSurface.getWidth(), (float)compositeSurface.getHeight()));
  Matrix4 projectionMatrix{};
  projectionMatrix.perspective(settings.perspective_fov, settings.m_viewportSize.x / settings.m_viewportSize.y, settings.perspective_nearz, settings.perspective_farz);
  m_viewport = KRViewport(settings.getViewportSize(), viewMatrix, projectionMatrix);
  m_viewport.setLODBias(settings.getLODBias());

  Vector3 vecCameraDirection = m_viewport.getCameraDirection();


  scene.updateOctree(m_viewport);
  
  m_viewport.expireOcclusionResults(getContext().getCurrentFrame());

  renderGraph.render(commandBuffer, compositeSurface, this);
}


void KRCamera::createBuffers(int renderBufferWidth, int renderBufferHeight)
{
  // TODO - Vulkan Refactoring..
/*
    if(renderBufferWidth != m_backingWidth || renderBufferHeight != m_backingHeight) {
        m_backingWidth = renderBufferWidth;
        m_backingHeight = renderBufferHeight;

        if (compositeDepthTexture) {
            GLDEBUG(glDeleteTextures(1, &compositeDepthTexture));
            compositeDepthTexture = 0;
        }

        if (compositeColorTexture) {
            GLDEBUG(glDeleteTextures(1, &compositeColorTexture));
            compositeColorTexture = 0;
        }

        if (lightAccumulationTexture) {
            GLDEBUG(glDeleteTextures(1, &lightAccumulationTexture));
            lightAccumulationTexture = 0;
        }

        if (compositeFramebuffer) {
            GLDEBUG(glDeleteFramebuffers(1, &compositeFramebuffer));
            compositeFramebuffer = 0;
        }

        if (lightAccumulationBuffer) {
            GLDEBUG(glDeleteFramebuffers(1, &lightAccumulationBuffer));
            lightAccumulationBuffer = 0;
        }


        // ===== Create offscreen compositing framebuffer object =====
        GLDEBUG(glGenFramebuffers(1, &compositeFramebuffer));
        GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, compositeFramebuffer));

        // ----- Create texture color buffer for compositeFramebuffer -----
        GLDEBUG(glGenTextures(1, &compositeColorTexture));
        GLDEBUG(glBindTexture(GL_TEXTURE_2D, compositeColorTexture));
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)); // This is necessary for non-power-of-two textures
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)); // This is necessary for non-power-of-two textures
        GLDEBUG(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_backingWidth, m_backingHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
        GLDEBUG(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, compositeColorTexture, 0));

        // ----- Create Depth Texture for compositeFramebuffer -----
        GLDEBUG(glGenTextures(1, &compositeDepthTexture));
        GLDEBUG(glBindTexture(GL_TEXTURE_2D, compositeDepthTexture));
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)); // This is necessary for non-power-of-two textures
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)); // This is necessary for non-power-of-two textures
        GLDEBUG(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_backingWidth, m_backingHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL));
        //GLDEBUG(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, m_backingWidth, m_backingHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL));
        //GLDEBUG(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, m_backingWidth, m_backingHeight));
        GLDEBUG(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, compositeDepthTexture, 0));

        // ===== Create offscreen compositing framebuffer object =====
        GLDEBUG(glGenFramebuffers(1, &lightAccumulationBuffer));
        GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, lightAccumulationBuffer));

        // ----- Create texture color buffer for compositeFramebuffer -----
        GLDEBUG(glGenTextures(1, &lightAccumulationTexture));
        GLDEBUG(glBindTexture(GL_TEXTURE_2D, lightAccumulationTexture));
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)); // This is necessary for non-power-of-two textures
        GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)); // This is necessary for non-power-of-two textures
        GLDEBUG(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_backingWidth, m_backingHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
        GLDEBUG(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightAccumulationTexture, 0));
    }

    int targetVolumetricBufferWidth = 0;
    int targetVolumetricBufferHeight = 0;
    if(settings.volumetric_environment_enable && settings.volumetric_environment_downsample != 0) {
        targetVolumetricBufferWidth = renderBufferWidth >> settings.volumetric_environment_downsample;
        targetVolumetricBufferHeight = renderBufferHeight >> settings.volumetric_environment_downsample;
    }


    if(targetVolumetricBufferWidth != volumetricBufferWidth || targetVolumetricBufferHeight != volumetricBufferHeight) {
        volumetricBufferWidth = targetVolumetricBufferWidth;
        volumetricBufferHeight = targetVolumetricBufferHeight;

        if (volumetricLightAccumulationTexture) {
            GLDEBUG(glDeleteTextures(1, &volumetricLightAccumulationTexture));
            volumetricLightAccumulationTexture = 0;
        }

        if (volumetricLightAccumulationBuffer) {
            GLDEBUG(glDeleteFramebuffers(1, &volumetricLightAccumulationBuffer));
            volumetricLightAccumulationBuffer = 0;
        }


        if(targetVolumetricBufferWidth != 0 && targetVolumetricBufferHeight != 0) {
            // ===== Create offscreen compositing framebuffer object for volumetric lighting =====
            GLDEBUG(glGenFramebuffers(1, &volumetricLightAccumulationBuffer));
            GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, volumetricLightAccumulationBuffer));

            // ----- Create texture color buffer for compositeFramebuffer for volumetric lighting  -----
            GLDEBUG(glGenTextures(1, &volumetricLightAccumulationTexture));
            GLDEBUG(glBindTexture(GL_TEXTURE_2D, volumetricLightAccumulationTexture));
            GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)); // This is necessary for non-power-of-two textures
            GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)); // This is necessary for non-power-of-two textures
            GLDEBUG(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, volumetricBufferWidth, volumetricBufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
            GLDEBUG(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, volumetricLightAccumulationTexture, 0));
        }
    }
    */
}

void KRCamera::destroyBuffers()
{
  // TODO - Vulkan Refactoring..
  /*
    if (compositeDepthTexture) {
        GLDEBUG(glDeleteTextures(1, &compositeDepthTexture));
        compositeDepthTexture = 0;
    }

    if (compositeColorTexture) {
    GLDEBUG(glDeleteTextures(1, &compositeColorTexture));
    compositeColorTexture = 0;
  }

    if (lightAccumulationTexture) {
        GLDEBUG(glDeleteTextures(1, &lightAccumulationTexture));
        lightAccumulationTexture = 0;
    }

    if (compositeFramebuffer) {
        GLDEBUG(glDeleteFramebuffers(1, &compositeFramebuffer));
        compositeFramebuffer = 0;
    }

    if (lightAccumulationBuffer) {
        GLDEBUG(glDeleteFramebuffers(1, &lightAccumulationBuffer));
        lightAccumulationBuffer = 0;
    }

    if (volumetricLightAccumulationTexture) {
        GLDEBUG(glDeleteTextures(1, &volumetricLightAccumulationTexture));
        volumetricLightAccumulationTexture = 0;
    }

    if (volumetricLightAccumulationBuffer) {
        GLDEBUG(glDeleteFramebuffers(1, &volumetricLightAccumulationBuffer));
        volumetricLightAccumulationBuffer = 0;
    }
    */
}

void KRCamera::renderPost(RenderInfo& ri)
{
  /*
   // TODO - Re-enable once post fx shader is converted for Vulkan
   KRMeshManager::KRVBOData& vertices = getContext().getMeshManager()->KRENGINE_VBO_DATA_2D_SQUARE_VERTICES;
   
   PipelineInfo info{};
   std::string shader_name("PostShader");
   info.shader_name = &shader_name;
   info.pCamera = this;
   info.renderPass = compositeSurface.getRenderPass(RenderPassType::RENDER_PASS_FORWARD_TRANSPARENT);
   info.rasterMode = RasterMode::kOpaqueNoTest;
   info.modelFormat = ModelFormat::KRENGINE_MODEL_FORMAT_STRIP;
   info.vertexAttributes = vertices.getVertexAttributes();
   
   KRPipeline *postShader = m_pContext->getPipelineManager()->getPipeline(surface, info);
   
   postShader->setPushConstant(KRPipeline::PushConstant::fade_color, m_fade_color);
   postShader->bind(commandBuffer, *this, m_viewport, Matrix4(), nullptr, nullptr, nullptr, compositeSurface.getRenderPass(RenderPassType::RENDER_PASS_FORWARD_TRANSPARENT));
   
   m_pContext->getTextureManager()->selectTexture(GL_TEXTURE_2D, 0, compositeDepthTexture);
   m_pContext->getTextureManager()->selectTexture(GL_TEXTURE_2D, 1, compositeColorTexture);
   
   if(settings.volumetric_environment_enable) {
   m_pContext->getTextureManager()->selectTexture(GL_TEXTURE_2D, 2, volumetricLightAccumulationTexture);
   }
   
   // Update attribute values.
   m_pContext->getMeshManager()->bindVBO(commandBuffer, &vertices, 1.0f);
   
   vkCmdDraw(commandBuffer, 4, 1, 0, 0);
   */
  
  
  //    if(bShowShadowBuffer) {
  //        KRPipeline *blitShader = m_pContext->getPipelineManager()->getShader("simple_blit", this, false, false, false, 0, false, false, false, false, false, false, false, false, false, false, false, false, false, compositeSurface.getRenderPass(RenderPassType::RENDER_PASS_FORWARD_TRANSPARENT));
  //
  //        for(int iShadow=0; iShadow < m_cShadowBuffers; iShadow++) {
  //            Matrix4 viewMatrix = Matrix4();
  //            viewMatrix.scale(0.20, 0.20, 0.20);
  //            viewMatrix.translate(-0.70, 0.70 - 0.45 * iShadow, 0.0);
  //            getContext().getPipelineManager()->selectShader(blitShader, KRViewport(getViewportSize(), viewMatrix, Matrix4()), shadowViewports, Matrix4(), Vector3(), NULL, 0, compositeSurface.getRenderPass(RenderPassType::RENDER_PASS_FORWARD_TRANSPARENT));
  //            m_pContext->getTextureManager()->selectTexture(1, NULL);
  //            m_pContext->getMeshManager()->bindVBO(&getContext().getMeshManager()->KRENGINE_VBO_DATA_2D_SQUARE_VERTICES);
  //            m_pContext->getTextureManager()->_setActiveTexture(0);
  //            GLDEBUG(glBindTexture(GL_TEXTURE_2D, shadowDepthTexture[iShadow]));
  //#if GL_EXT_shadow_samplers
  //            GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_EXT, GL_NONE)); // TODO - Detect GL_EXT_shadow_samplers and only activate if available
  //#endif
  //            GLDEBUG(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  //#if GL_EXT_shadow_samplers
  //            GLDEBUG(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_EXT, GL_COMPARE_REF_TO_TEXTURE_EXT)); // TODO - Detect GL_EXT_shadow_samplers and only activate if available
  //#endif
  //        }
  //
  //        m_pContext->getTextureManager()->selectTexture(0, NULL);
  //        m_pContext->getTextureManager()->_setActiveTexture(0);
  //        GLDEBUG(glBindTexture(GL_TEXTURE_2D, 0));
  //    }
  
}

void KRCamera::renderDebug(RenderInfo& ri)
{
  const char* szText = settings.m_debug_text.c_str();

  std::string debug_text;
  if (settings.debug_display != KRRenderSettings::KRENGINE_DEBUG_DISPLAY_NONE) {
    debug_text = getDebugText();;
    if (debug_text.length() > 0) {
      szText = debug_text.c_str();
    }
  }

  if (*szText) {
    int row_count = 1;
    const int MAX_TABS = 5;
    const int TAB_EXTRA = 2;
    int tab_cols[MAX_TABS] = { 0, 0, 0, 0, 0 };
    int iCol = 0;
    int iTab = 0;
    const char* pChar = szText;
    while (*pChar) {
      char c = *pChar++;
      if (c == '\n') {
        row_count++;
        iCol = 0;
        iTab = 0;
      } else if (c == '\t') {
        iCol = 0;
        iTab++;
      } else {
        iCol++;
        if (iCol > tab_cols[iTab]) tab_cols[iTab] = iCol;
      }
    }

    iCol = 0;
    for (iTab = 0; iTab < MAX_TABS; iTab++) {
      iCol += tab_cols[iTab] + TAB_EXTRA;
      tab_cols[iTab] = iCol;
    }

    const int DEBUG_TEXT_COLUMNS = 256;
    const int DEBUG_TEXT_ROWS = 128;

    if (m_debug_text_vertices.getSize() == 0) {
      m_debug_text_vertices.expand(sizeof(DebugTextVertexData) * DEBUG_TEXT_COLUMNS * DEBUG_TEXT_ROWS * 6);
    }
    int vertex_count = 0;

    m_debug_text_vertices.lock();
    DebugTextVertexData* vertex_data = (DebugTextVertexData*)m_debug_text_vertices.getStart();

    pChar = szText;
    float dScaleX = 2.0f / (1024.0f / 16.0f);
    float dScaleY = 2.0f / (768.0f / 16.0f);
    float dTexScale = 1.0f / 16.0f;
    int iRow = row_count - 1; iCol = 0; iTab = 0;
    while (*pChar) {
      char c = *pChar++;
      if (c == '\n') {
        iCol = 0;
        iTab = 0;
        iRow--;
      } else if (c == '\t') {
        iCol = tab_cols[iTab++];
      } else {
        if (iCol < DEBUG_TEXT_COLUMNS && iRow < DEBUG_TEXT_ROWS) {
          int iChar = c - '\0';
          int iTexCol = iChar % 16;
          int iTexRow = 15 - (iChar - iTexCol) / 16;

          Vector2 top_left_pos = Vector2::Create(-1.0f + dScaleX * iCol, dScaleY * iRow - 1.0f);
          Vector2 bottom_right_pos = Vector2::Create(-1.0f + dScaleX * (iCol + 1), dScaleY * iRow + dScaleY - 1.0f);
          top_left_pos += Vector2::Create(1.0f / 2048.0f * 0.5f, 1.0f / 1536.0f * 0.5f);
          bottom_right_pos += Vector2::Create(1.0f / 2048.0f * 0.5f, 1.0f / 1536.0f * 0.5f);
          Vector2 top_left_uv = Vector2::Create(dTexScale * iTexCol, dTexScale * iTexRow + dTexScale);
          Vector2 bottom_right_uv = Vector2::Create(dTexScale * iTexCol + dTexScale, dTexScale * iTexRow);

          vertex_data[vertex_count].x = top_left_pos.x;
          vertex_data[vertex_count].y = top_left_pos.y;
          vertex_data[vertex_count].z = 0.0f;
          vertex_data[vertex_count].u = top_left_uv.x;
          vertex_data[vertex_count].v = top_left_uv.y;
          vertex_count++;

          vertex_data[vertex_count].x = bottom_right_pos.x;
          vertex_data[vertex_count].y = bottom_right_pos.y;
          vertex_data[vertex_count].z = 0.0f;
          vertex_data[vertex_count].u = bottom_right_uv.x;
          vertex_data[vertex_count].v = bottom_right_uv.y;
          vertex_count++;

          vertex_data[vertex_count].x = top_left_pos.x;
          vertex_data[vertex_count].y = bottom_right_pos.y;
          vertex_data[vertex_count].z = 0.0f;
          vertex_data[vertex_count].u = top_left_uv.x;
          vertex_data[vertex_count].v = bottom_right_uv.y;
          vertex_count++;


          vertex_data[vertex_count].x = top_left_pos.x;
          vertex_data[vertex_count].y = top_left_pos.y;
          vertex_data[vertex_count].z = 0.0f;
          vertex_data[vertex_count].u = top_left_uv.x;
          vertex_data[vertex_count].v = top_left_uv.y;
          vertex_count++;

          vertex_data[vertex_count].x = bottom_right_pos.x;
          vertex_data[vertex_count].y = top_left_pos.y;
          vertex_data[vertex_count].z = 0.0f;
          vertex_data[vertex_count].u = bottom_right_uv.x;
          vertex_data[vertex_count].v = top_left_uv.y;
          vertex_count++;

          vertex_data[vertex_count].x = bottom_right_pos.x;
          vertex_data[vertex_count].y = bottom_right_pos.y;
          vertex_data[vertex_count].z = 0.0f;
          vertex_data[vertex_count].u = bottom_right_uv.x;
          vertex_data[vertex_count].v = bottom_right_uv.y;
          vertex_count++;
        }

        iCol++;
      }
    }
    
    m_debug_text_vbo_data.load(ri.commandBuffer);
    

    KRTexture* fontTexture = m_pContext->getTextureManager()->getTexture("font");
    fontTexture->resetPoolExpiry(0.0f, KRTexture::TEXTURE_USAGE_UI);
    if (fontTexture->getStreamLevel(KRTexture::TEXTURE_USAGE_UI) != kraken_stream_level::STREAM_LEVEL_OUT) {
      
      PipelineInfo info{};
      std::string shader_name("debug_font");
      info.shader_name = &shader_name;
      info.pCamera = this;
      info.renderPass = ri.renderPass;
      info.rasterMode = RasterMode::kAlphaBlendNoTest;
      info.cullMode = CullMode::kCullNone;
      info.vertexAttributes = (1 << KRMesh::KRENGINE_ATTRIB_VERTEX) | (1 << KRMesh::KRENGINE_ATTRIB_TEXUVA);
      info.modelFormat = ModelFormat::KRENGINE_MODEL_FORMAT_TRIANGLES;
      KRPipeline* fontShader = m_pContext->getPipelineManager()->getPipeline(*ri.surface, info);
      
      fontShader->setImageBinding("fontTexture", fontTexture, getContext().getSamplerManager()->DEFAULT_CLAMPED_SAMPLER);
      fontShader->bind(ri, Matrix4());
      
      m_debug_text_vbo_data.bind(ri.commandBuffer);
      
      vkCmdDraw(ri.commandBuffer, vertex_count, 1, 0, 0);
    }

    m_debug_text_vertices.unlock();

  } else {
    if (m_debug_text_vertices.getSize() > 0) {
      m_debug_text_vertices = Block();
    }
  }
}


std::string KRCamera::getDebugText()
{
  std::stringstream stream;
  stream.precision(std::numeric_limits<long double>::digits10);



  uint64_t fps = 0;
  if (m_frame_times_filled == KRAKEN_FPS_AVERAGE_FRAME_COUNT) {
    for (int i = 0; i < KRAKEN_FPS_AVERAGE_FRAME_COUNT; i++) {
      fps += m_frame_times[i];
    }
    fps = 1000000 / (fps / KRAKEN_FPS_AVERAGE_FRAME_COUNT); // Order of division chosen to prevent overflow
  }

  switch (settings.debug_display) {
  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_NONE: // ----====---- No debug display ----====----
    break;

  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_TIME: // ----====---- Time / FPS ----====----
  {
    if (fps > 0) {
      stream << "FPS\t" << fps;
    }
  }
  break;

  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_MEMORY: // ----====---- Memory Utilization ----=====----
  {
#if defined(__APPLE__)
    // ---- CPU Memory ----

    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);
    kern_return_t kerr = task_info(mach_task_self(),
                                   TASK_BASIC_INFO,
                                   (task_info_t)&info,
                                   &size);
    if (kerr == KERN_SUCCESS) {
      stream << "\tResident\tVirtual\tTotal";
      stream << "\nCPU\t" << (info.resident_size / 1024 / 1024) << " MB\t" << (info.virtual_size / 1024 / 1024) << " MB\t" << ((info.resident_size + info.virtual_size) / 1024 / 1024) << " MB";
    } else {
      stream << "\nERROR: Could not get CPU memory utilization.";
    }


    mach_port_t host_port = mach_host_self();
    mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
    vm_size_t pagesize = 0;
    vm_statistics_data_t vm_stat;
    if (host_page_size(host_port, &pagesize) != KERN_SUCCESS) {
      stream << "\n\nERROR: Could not get VM page size.";
    } else if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS) {
      stream << "\n\nERROR: Could not get VM stats.";
    } else {
      stream << "\n\n\n\tWired\tActive\tInactive\tFree\tTotal";
      stream << "\nVM\t";
      stream << (vm_stat.wire_count * pagesize / 1024 / 1024) << " MB\t";
      stream << (vm_stat.active_count * pagesize / 1024 / 1024) << " MB\t";
      stream << (vm_stat.inactive_count * pagesize / 1024 / 1024) << " MB\t";
      stream << (vm_stat.free_count * pagesize / 1024 / 1024) << " MB\t";
      stream << ((vm_stat.wire_count + vm_stat.active_count + vm_stat.inactive_count) * pagesize / 1024 / 1024) << " MB";
    }
#endif // defined(__APPLE__)

    // ---- GPU Memory ----
    size_t texture_count_active = m_pContext->getTextureManager()->getActiveTextures().size();
    size_t texture_count = texture_count_active;
    long texture_mem_active = m_pContext->getTextureManager()->getMemActive();
    long texture_mem_used = m_pContext->getTextureManager()->getMemUsed();
    long texture_mem_throughput = m_pContext->getTextureManager()->getMemoryTransferedThisFrame();

    size_t vbo_count_active = m_pContext->getMeshManager()->getActiveVBOCount();
    long vbo_mem_active = m_pContext->getMeshManager()->getMemActive();
    long vbo_mem_used = m_pContext->getMeshManager()->getMemUsed();
    long vbo_mem_throughput = m_pContext->getMeshManager()->getMemoryTransferedThisFrame();

    long total_mem_active = texture_mem_active + vbo_mem_active;
    long total_mem_used = texture_mem_used + vbo_mem_used;
    long total_mem_throughput = texture_mem_throughput + vbo_mem_throughput;

    stream << "\n\n\n\t# Active\t# Used\tActive\tUsed\tThroughput\n";

    stream << "Textures\t" << texture_count_active << "\t" << texture_count << "\t" << (texture_mem_active / 1024) << " KB\t" << (texture_mem_used / 1024) << " KB\t" << (texture_mem_throughput / 1024) << " KB / frame\n";
    stream << "VBO's\t" << vbo_count_active << "\t" << vbo_count_active << "\t" << (vbo_mem_active / 1024) << " KB\t" << (vbo_mem_used / 1024) << " KB\t" << (vbo_mem_throughput / 1024) << " KB / frame\n";
    stream << "\nGPU Total\t\t\t" << (total_mem_active / 1024) << " KB\t" << (total_mem_used / 1024) << " KB\t" << (total_mem_throughput / 1024) << " KB / frame";
  }
  break;

  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_TEXTURES: // ----====---- List Active Textures ----====----
  {
    bool first = true;
    int texture_count = 0;
    std::set<KRTexture*> active_textures = m_pContext->getTextureManager()->getActiveTextures();
    for (std::set<KRTexture*>::iterator itr = active_textures.begin(); itr != active_textures.end(); itr++) {
      KRTexture* texture = *itr;
      if (first) {
        first = false;
      } else {
        stream << "\n";
      }
      stream << texture->getName();
      stream << "\t";
      stream << texture->getMemSize() / 1024;
      stream << " KB";
      stream << "\t";
      stream << texture->getMaxMipMap();
      if (texture->hasMipmaps() && texture->getCurrentLodMaxDim() != texture->getMaxMipMap()) {
        stream << " px => ";
        stream << texture->getCurrentLodMaxDim();
      }
      stream << " px";
      texture_count++;
    }

    stream << "\n\nTOTAL: ";
    stream << texture_count;
    stream << " textures\t";
    stream << (m_pContext->getTextureManager()->getMemActive() / 1024) << " KB";
  }
  break;

  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_DRAW_CALLS: // ----====---- List Draw Calls ----====----
  {
    std::vector<KRMeshManager::draw_call_info> draw_calls = m_pContext->getMeshManager()->getDrawCalls();

    long draw_call_count = 0;
    long vertex_count = 0;
    stream << "\tVerts\tPass\tObject\tMaterial";
    for (std::vector<KRMeshManager::draw_call_info>::iterator itr = draw_calls.begin(); itr != draw_calls.end(); itr++) {
      draw_call_count++;
      stream << "\n" << draw_call_count << "\t" << (*itr).vertex_count << "\t";
      switch ((*itr).pass) {
      case RenderPassType::RENDER_PASS_FORWARD_OPAQUE:
        stream << "opaq";
        break;
      case RenderPassType::RENDER_PASS_DEFERRED_GBUFFER:
        stream << "d gb";
        break;
      case RenderPassType::RENDER_PASS_DEFERRED_LIGHTS:
        stream << "d light";
        break;
      case RenderPassType::RENDER_PASS_DEFERRED_OPAQUE:
        stream << "d opaq";
        break;
      case RenderPassType::RENDER_PASS_FORWARD_TRANSPARENT:
        stream << "trans";
        break;
      case RenderPassType::RENDER_PASS_PARTICLE_OCCLUSION:
        stream << "p occl";
        break;
      case RenderPassType::RENDER_PASS_ADDITIVE_PARTICLES:
        stream << "a part";
        break;
      case RenderPassType::RENDER_PASS_VOLUMETRIC_EFFECTS_ADDITIVE:
        stream << "vol add";
        break;
      case RenderPassType::RENDER_PASS_SHADOWMAP:
        stream << "shadow";
        break;
      default:
        // Suppress warnings
        break;
      }
      stream << "\t" << (*itr).object_name << "\t" << (*itr).material_name;
      vertex_count += (*itr).vertex_count;
    }
    stream << "\n\n\t\tTOTAL:\t" << draw_call_count << " draw calls\t" << vertex_count << " vertices";
  }
  break;
  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_OCTREE:
    stream << "Octree Visualization";
    break;
  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_COLLIDERS:
    stream << "Collider Visualization";
    break;
  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_BONES:
    stream << "Bone Visualization";
  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_SIREN_REVERB_ZONES:
    stream << "Siren - Reverb Zones";
    break;
  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_SIREN_AMBIENT_ZONES:
    stream << "Siren - Ambient Zones";
    break;
  case KRRenderSettings::KRENGINE_DEBUG_DISPLAY_NUMBER:
    // Suppress warning
    break;
  }
  return stream.str();
}


KRViewport* KRCamera::getViewport()
{
  return &m_viewport;
}

void KRCamera::setFadeColor(const Vector4& fade_color)
{
  m_fade_color = fade_color;
}

Vector4 KRCamera::getFadeColor()
{
  return m_fade_color;
}
