//
//  KRAmbientZone.cpp
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

#include "KRAmbientZone.h"
#include "KRContext.h"
#include "KRRenderPass.h"

using namespace hydra;

/* static */
void KRAmbientZone::InitNodeInfo(KrNodeInfo* nodeInfo)
{
  KRNode::InitNodeInfo(nodeInfo);
  nodeInfo->ambient_zone.gain = 1.0f;
  nodeInfo->ambient_zone.gradient = 0.25f;
  nodeInfo->ambient_zone.pZoneName = nullptr;
  nodeInfo->ambient_zone.sample = -1;
}

KRAmbientZone::KRAmbientZone(KRScene& scene, std::string name) : KRNode(scene, name)
{
  m_ambient = "";
  m_ambient_gain = 1.0f;

  m_gradient_distance = 0.25f;

}

KRAmbientZone::~KRAmbientZone()
{}

std::string KRAmbientZone::getElementName()
{
  return "ambient_zone";
}

tinyxml2::XMLElement* KRAmbientZone::saveXML(tinyxml2::XMLNode* parent)
{
  tinyxml2::XMLElement* e = KRNode::saveXML(parent);
  e->SetAttribute("zone", m_zone.c_str());
  e->SetAttribute("sample", m_ambient.c_str());
  e->SetAttribute("gain", m_ambient_gain);
  e->SetAttribute("gradient", m_gradient_distance);
  return e;
}

void KRAmbientZone::loadXML(tinyxml2::XMLElement* e)
{
  KRNode::loadXML(e);

  m_zone = e->Attribute("zone");

  m_gradient_distance = 0.25f;
  if (e->QueryFloatAttribute("gradient", &m_gradient_distance) != tinyxml2::XML_SUCCESS) {
    m_gradient_distance = 0.25f;
  }

  m_ambient = e->Attribute("sample");

  m_ambient_gain = 1.0f;
  if (e->QueryFloatAttribute("gain", &m_ambient_gain) != tinyxml2::XML_SUCCESS) {
    m_ambient_gain = 1.0f;
  }
}

std::string KRAmbientZone::getAmbient()
{
  return m_ambient;
}

void KRAmbientZone::setAmbient(const std::string& ambient)
{
  m_ambient = ambient;
}

float KRAmbientZone::getAmbientGain()
{
  return m_ambient_gain;
}

void KRAmbientZone::setAmbientGain(float ambient_gain)
{
  m_ambient_gain = ambient_gain;
}

std::string KRAmbientZone::getZone()
{
  return m_zone;
}

void KRAmbientZone::setZone(const std::string& zone)
{
  m_zone = zone;
}

void KRAmbientZone::render(RenderInfo& ri)
{
  if (m_lod_visible <= LOD_VISIBILITY_PRESTREAM) return;

  KRNode::render(ri);

  bool bVisualize = ri.camera->settings.debug_display == KRRenderSettings::KRENGINE_DEBUG_DISPLAY_SIREN_AMBIENT_ZONES;

  if (ri.renderPass->getType() == RenderPassType::RENDER_PASS_FORWARD_TRANSPARENT && bVisualize) {
    KRMesh* sphereModel = getContext().getMeshManager()->getMaxLODModel("__sphere");
    if (sphereModel) {

      Matrix4 sphereModelMatrix = getModelMatrix();

      PipelineInfo info{};
      std::string shader_name("visualize_overlay");
      info.shader_name = &shader_name;
      info.pCamera = ri.camera;
      info.point_lights = &ri.point_lights;
      info.directional_lights = &ri.directional_lights;
      info.spot_lights = &ri.spot_lights;
      info.renderPass = ri.renderPass;
      info.rasterMode = RasterMode::kAdditive;
      info.modelFormat = sphereModel->getModelFormat();
      info.vertexAttributes = sphereModel->getVertexAttributes();

      KRPipeline* pPipeline = getContext().getPipelineManager()->getPipeline(*ri.surface, info);
      pPipeline->bind(ri, sphereModelMatrix);

      sphereModel->renderNoMaterials(ri.commandBuffer, ri.renderPass, getName(), "visualize_overlay", 1.0f);
    } // sphereModel
  }
}


float KRAmbientZone::getGradientDistance()
{
  return m_gradient_distance;
}

void KRAmbientZone::setGradientDistance(float gradient_distance)
{
  m_gradient_distance = gradient_distance;
}

AABB KRAmbientZone::getBounds()
{
  // Ambient zones always have a -1, -1, -1 to 1, 1, 1 bounding box
  return AABB::Create(-Vector3::One(), Vector3::One(), getModelMatrix());
}

float KRAmbientZone::getContainment(const Vector3& pos)
{
  AABB bounds = getBounds();
  if (bounds.contains(pos)) {
    Vector3 size = bounds.size();
    Vector3 diff = pos - bounds.center();
    diff = diff * 2.0f;
    diff = Vector3::Create(diff.x / size.x, diff.y / size.y, diff.z / size.z);
    float d = diff.magnitude();

    if (m_gradient_distance <= 0.0f) {
      // Avoid division by zero
      d = d > 1.0f ? 0.0f : 1.0f;
    } else {
      d = (1.0f - d) / m_gradient_distance;
      d = KRCLAMP(d, 0.0f, 1.0f);
    }
    return d;

  } else {
    return 0.0f;
  }
}
