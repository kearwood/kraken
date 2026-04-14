//
//  KRMaterial.cpp
//  Kraken Engine
//
//  Copyright 2025 Kearwood Gilbert. All rights reserved.
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

#include "KRMaterial.h"
#include "resources/texture/KRTextureManager.h"
#include "KRRenderPass.h"

#include "KRContext.h"

using namespace mimir;
using namespace hydra;

KRMaterial::KRMaterial(KRContext& context, const char* szName)
  : KRResource(context, szName)
  , m_ambient{ KRTexture::TEXTURE_USAGE_AMBIENT_MAP, 0, Vector2::Create(0.0f, 0.0f), Vector2::Create(1.0f, 1.0f) }
  , m_diffuse{ KRTexture::TEXTURE_USAGE_DIFFUSE_MAP, 0, Vector2::Create(0.0f, 0.0f), Vector2::Create(1.0f, 1.0f) }
  , m_specular{ KRTexture::TEXTURE_USAGE_NORMAL_MAP, 0, Vector2::Create(0.0f, 0.0f), Vector2::Create(1.0f, 1.0f) }
  , m_reflection{ KRTexture::TEXTURE_USAGE_REFLECTION_MAP, 0, Vector2::Create(0.0f, 0.0f), Vector2::Create(1.0f, 1.0f) }
  , m_reflectionCube(KRTexture::TEXTURE_USAGE_REFECTION_CUBE)
  , m_normal{ KRTexture::TEXTURE_USAGE_NORMAL_MAP, 0, Vector2::Create(0.0f, 0.0f), Vector2::Create(1.0f, 1.0f) }
{
  m_name = szName;
  m_ambientColor = Vector3::Zero();
  m_diffuseColor = Vector3::One();
  m_specularColor = Vector3::One();
  m_reflectionColor = Vector3::Zero();
  m_tr = 1.0f;
  m_ns = 0.0f;
  
  m_alpha_mode = KRMATERIAL_ALPHA_MODE_OPAQUE;
}

KRMaterial::~KRMaterial()
{

}

std::string KRMaterial::getExtension()
{
  return "mtl";
}

bool KRMaterial::needsVertexTangents()
{
  return m_normal.texture.isSet();
}

bool KRMaterial::save(Block& data)
{
  std::stringstream stream;
  stream.precision(std::numeric_limits<long double>::digits10);
  stream.setf(std::ios::fixed, std::ios::floatfield);

  stream << "newmtl " << m_name;
  stream << "\nka " << m_ambientColor.x << " " << m_ambientColor.y << " " << m_ambientColor.z;
  stream << "\nkd " << m_diffuseColor.x << " " << m_diffuseColor.y << " " << m_diffuseColor.z;
  stream << "\nks " << m_specularColor.x << " " << m_specularColor.y << " " << m_specularColor.z;
  stream << "\nkr " << m_reflectionColor.x << " " << m_reflectionColor.y << " " << m_reflectionColor.z;
  stream << "\nTr " << m_tr;
  stream << "\nNs " << m_ns;
  if (m_ambient.texture.isSet()) {
    stream << "\nmap_Ka " << m_ambient.texture.getName() << ".pvr -s " << m_ambient.scale.x << " " << m_ambient.scale.y << " -o " << m_ambient.offset.x << " " << m_ambient.offset.y;
  } else {
    stream << "\n# map_Ka filename.pvr -s 1.0 1.0 -o 0.0 0.0";
  }
  if (m_diffuse.texture.isSet()) {
    stream << "\nmap_Kd " << m_diffuse.texture.getName() << ".pvr -s " << m_diffuse.scale.x << " " << m_diffuse.scale.y << " -o " << m_diffuse.offset.x << " " << m_diffuse.offset.y;
  } else {
    stream << "\n# map_Kd filename.pvr -s 1.0 1.0 -o 0.0 0.0";
  }
  if (m_specular.texture.isSet()) {
    stream << "\nmap_Ks " << m_specular.texture.getName() << ".pvr -s " << m_specular.scale.x << " " << m_specular.scale.y << " -o " << m_specular.offset.x << " " << m_specular.offset.y << "\n";
  } else {
    stream << "\n# map_Ks filename.pvr -s 1.0 1.0 -o 0.0 0.0";
  }
  if (m_normal.texture.isSet()) {
    stream << "\nmap_Normal " << m_normal.texture.getName() << ".pvr -s " << m_normal.scale.x << " " << m_normal.scale.y << " -o " << m_normal.offset.x << " " << m_normal.offset.y;
  } else {
    stream << "\n# map_Normal filename.pvr -s 1.0 1.0 -o 0.0 0.0";
  }
  if (m_reflection.texture.isSet()) {
    stream << "\nmap_Reflection " << m_reflection.texture.getName() << ".pvr -s " << m_reflection.scale.x << " " << m_reflection.scale.y << " -o " << m_reflection.offset.x << " " << m_reflection.offset.y;
  } else {
    stream << "\n# map_Reflection filename.pvr -s 1.0 1.0 -o 0.0 0.0";
  }
  if (m_reflectionCube.isSet()) {
    stream << "\nmap_ReflectionCube " << m_reflectionCube.getName() << ".pvr";
  } else {
    stream << "\n# map_ReflectionCube cubemapname";
  }
  switch (m_alpha_mode) {
  case KRMATERIAL_ALPHA_MODE_OPAQUE:
    stream << "\nalpha_mode opaque";
    break;
  case KRMATERIAL_ALPHA_MODE_TEST:
    stream << "\nalpha_mode test";
    break;
  case KRMATERIAL_ALPHA_MODE_BLENDONESIDE:
    stream << "\nalpha_mode blendoneside";
    break;
  case KRMATERIAL_ALPHA_MODE_BLENDTWOSIDE:
    stream << "\nalpha_mode blendtwoside";
    break;
  }
  stream << "\n# alpha_mode opaque, test, blendoneside, or blendtwoside";

  stream << "\n";
  data.append(stream.str());

  return true;
}

void KRMaterial::setAmbientMap(std::string texture_name, Vector2 texture_scale, Vector2 texture_offset)
{
  m_ambient.texture.set(texture_name);
  m_ambient.scale = texture_scale;
  m_ambient.offset = texture_offset;
}

void KRMaterial::setDiffuseMap(std::string texture_name, Vector2 texture_scale, Vector2 texture_offset)
{
  m_diffuse.texture.set(texture_name);
  m_diffuse.scale = texture_scale;
  m_diffuse.offset = texture_offset;
}

void KRMaterial::setSpecularMap(std::string texture_name, Vector2 texture_scale, Vector2 texture_offset)
{
  m_specular.texture.set(texture_name);
  m_specular.scale = texture_scale;
  m_specular.offset = texture_offset;
}

void KRMaterial::setNormalMap(std::string texture_name, Vector2 texture_scale, Vector2 texture_offset)
{
  m_normal.texture.set(texture_name);
  m_normal.scale = texture_scale;
  m_normal.offset = texture_offset;
}

void KRMaterial::setReflectionMap(std::string texture_name, Vector2 texture_scale, Vector2 texture_offset)
{
  m_reflection.texture.set(texture_name);
  m_reflection.scale = texture_scale;
  m_reflection.offset = texture_offset;
}

void KRMaterial::setReflectionCube(std::string texture_name)
{
  m_reflectionCube.set(texture_name);
}

void KRMaterial::setAlphaMode(KRMaterial::alpha_mode_type alpha_mode)
{
  m_alpha_mode = alpha_mode;
}

KRMaterial::alpha_mode_type KRMaterial::getAlphaMode()
{
  return m_alpha_mode;
}

void KRMaterial::setAmbient(const Vector3& c)
{
  m_ambientColor = c;
}

void KRMaterial::setDiffuse(const Vector3& c)
{
  m_diffuseColor = c;
}

void KRMaterial::setSpecular(const Vector3& c)
{
  m_specularColor = c;
}

void KRMaterial::setReflection(const Vector3& c)
{
  m_reflectionColor = c;
}

void KRMaterial::setTransparency(float a)
{
  if (a < 1.0f && m_alpha_mode == KRMaterial::KRMATERIAL_ALPHA_MODE_OPAQUE) {
    setAlphaMode(KRMaterial::KRMATERIAL_ALPHA_MODE_BLENDONESIDE);
  }
  m_tr = a;
}

void KRMaterial::setShininess(float s)
{
  m_ns = s;
}

bool KRMaterial::isTransparent()
{
  return m_tr < 1.0 || m_alpha_mode == KRMATERIAL_ALPHA_MODE_BLENDONESIDE || m_alpha_mode == KRMATERIAL_ALPHA_MODE_BLENDTWOSIDE;
}

void KRMaterial::getResourceBindings(std::list<KRResourceBinding*>& bindings)
{
  KRResource::getResourceBindings(bindings);

  bindings.push_back(&m_ambient.texture);
  bindings.push_back(&m_diffuse.texture);
  bindings.push_back(&m_normal.texture);
  bindings.push_back(&m_specular.texture);
  bindings.push_back(&m_reflection.texture);
  bindings.push_back(&m_reflectionCube);
}

kraken_stream_level KRMaterial::getStreamLevel()
{
  kraken_stream_level stream_level = kraken_stream_level::STREAM_LEVEL_IN_HQ;

  if (m_ambient.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_ambient.texture.get()->getStreamLevel());
  }

  if (m_diffuse.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_diffuse.texture.get()->getStreamLevel());
  }

  if (m_normal.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_normal.texture.get()->getStreamLevel());
  }

  if (m_specular.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_specular.texture.get()->getStreamLevel());
  }

  if (m_reflection.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_reflection.texture.get()->getStreamLevel());
  }

  if (m_reflectionCube.isBound()) {
    stream_level = KRMIN(stream_level, m_reflectionCube.get()->getStreamLevel());
  }

  return stream_level;
}

void KRMaterial::bind(KRNode::RenderInfo& ri, ModelFormat modelFormat, __uint32_t vertexAttributes, CullMode cullMode, const std::vector<KRBone*>& bones, const std::vector<Matrix4>& bind_poses, const Matrix4& matModel, KRTexture* pLightMap, float lod_coverage)
{
  bool bLightMap = pLightMap && ri.camera->settings.bEnableLightMap;

  Vector2 default_scale = Vector2::One();
  Vector2 default_offset = Vector2::Zero();

  bool bHasReflection = m_reflectionColor != Vector3::Zero();
  bool bDiffuseMap = m_diffuse.texture.isBound() && ri.camera->settings.bEnableDiffuseMap;
  bool bNormalMap = m_normal.texture.isBound() && ri.camera->settings.bEnableNormalMap;
  bool bSpecMap = m_specular.texture.isBound() && ri.camera->settings.bEnableSpecMap;
  bool bReflectionMap = m_reflection.texture.isBound() && ri.camera->settings.bEnableReflectionMap && ri.camera->settings.bEnableReflection && bHasReflection;
  bool bReflectionCubeMap = m_reflectionCube.isBound() && ri.camera->settings.bEnableReflection && bHasReflection;
  bool bAlphaTest = (m_alpha_mode == KRMATERIAL_ALPHA_MODE_TEST) && bDiffuseMap;
  bool bAlphaBlend = (m_alpha_mode == KRMATERIAL_ALPHA_MODE_BLENDONESIDE) || (m_alpha_mode == KRMATERIAL_ALPHA_MODE_BLENDTWOSIDE);

  PipelineInfo info{};
  std::string shader_name("object");
  info.shader_name = &shader_name;
  info.pCamera = ri.camera;
  info.point_lights = &ri.point_lights;
  info.directional_lights = &ri.directional_lights;
  info.spot_lights = &ri.spot_lights;
  info.bone_count = (int)bones.size();
  info.renderPass = ri.renderPass;
  info.bDiffuseMap = bDiffuseMap;
  info.bNormalMap = bNormalMap;
  info.bSpecMap = bSpecMap;
  info.bReflectionMap = bReflectionMap;
  info.bReflectionCubeMap = bReflectionCubeMap;
  info.bLightMap = bLightMap;
  info.bDiffuseMapScale = m_diffuse.scale != default_scale && bDiffuseMap;
  info.bSpecMapScale = m_specular.scale != default_scale && bSpecMap;
  info.bNormalMapScale = m_normal.scale != default_scale && bNormalMap;
  info.bReflectionMapScale = m_reflection.scale != default_scale && bReflectionMap;
  info.bDiffuseMapOffset = m_diffuse.offset != default_offset && bDiffuseMap;
  info.bSpecMapOffset = m_specular.offset != default_offset && bSpecMap;
  info.bNormalMapOffset = m_normal.offset != default_offset && bNormalMap;
  info.bReflectionMapOffset = m_reflection.offset != default_offset && bReflectionMap;
  info.bAlphaTest = bAlphaTest;
  info.rasterMode = bAlphaBlend ? RasterMode::kAlphaBlend : RasterMode::kOpaque;
  info.renderPass = ri.renderPass;
  info.modelFormat = modelFormat;
  info.vertexAttributes = vertexAttributes;
  info.cullMode = cullMode;
  KRPipeline* pShader = getContext().getPipelineManager()->getPipeline(*ri.surface, info);

  // Bind bones
  if (pShader->hasPushConstant(ShaderValue::bone_transforms)) {
    float bone_mats[256 * 16];
    float* bone_mat_component = bone_mats;
    for (int bone_index = 0; bone_index < bones.size(); bone_index++) {
      KRBone* bone = bones[bone_index];

      //                Vector3 initialRotation = bone->getInitialLocalRotation();
      //                Vector3 rotation = bone->getLocalRotation();
      //                Vector3 initialTranslation = bone->getInitialLocalTranslation();
      //                Vector3 translation = bone->getLocalTranslation();
      //                Vector3 initialScale = bone->getInitialLocalScale();
      //                Vector3 scale = bone->getLocalScale();
      //                
                  //printf("%s - delta rotation: %.4f %.4f %.4f\n", bone->getName().c_str(), (rotation.x - initialRotation.x) * 180.0 / M_PI, (rotation.y - initialRotation.y) * 180.0 / M_PI, (rotation.z - initialRotation.z) * 180.0 / M_PI);
                  //printf("%s - delta translation: %.4f %.4f %.4f\n", bone->getName().c_str(), translation.x - initialTranslation.x, translation.y - initialTranslation.y, translation.z - initialTranslation.z);
      //                printf("%s - delta scale: %.4f %.4f %.4f\n", bone->getName().c_str(), scale.x - initialScale.x, scale.y - initialScale.y, scale.z - initialScale.z);

      Matrix4 skin_bone_bind_pose = bind_poses[bone_index];
      Matrix4 active_mat = bone->getActivePoseMatrix();
      Matrix4 inv_bind_mat = bone->getInverseBindPoseMatrix();
      Matrix4 inv_bind_mat2 = Matrix4::Invert(bind_poses[bone_index]);
      Matrix4 t = (inv_bind_mat * active_mat);
      Matrix4 t2 = inv_bind_mat2 * bone->getModelMatrix();
      for (int i = 0; i < 16; i++) {
        *bone_mat_component++ = t[i];
      }
    }
    if (pShader->hasPushConstant(ShaderValue::bone_transforms)) {
      pShader->setPushConstant(ShaderValue::bone_transforms, (Matrix4*)bone_mats, bones.size());
    }
  }

  if (bDiffuseMap) {
    pShader->setImageBinding("diffuseTexture", m_diffuse.texture.get(), getContext().getSamplerManager()->DEFAULT_WRAPPING_SAMPLER);
  }

  if (bSpecMap) {
    pShader->setImageBinding("specularTexture", m_specular.texture.get(), getContext().getSamplerManager()->DEFAULT_WRAPPING_SAMPLER);
  }

  if (bNormalMap) {
    pShader->setImageBinding("normalTexture", m_normal.texture.get(), getContext().getSamplerManager()->DEFAULT_WRAPPING_SAMPLER);
  }

  if (bReflectionCubeMap) {
    pShader->setImageBinding("reflectionCubeTexture", m_reflectionCube.get(), getContext().getSamplerManager()->DEFAULT_CLAMPED_SAMPLER);
  }

  if (bReflectionMap) {
    pShader->setImageBinding("reflectionTexture", m_reflection.texture.get(), getContext().getSamplerManager()->DEFAULT_CLAMPED_SAMPLER);
  }

  ri.reflectedObjects.push_back(this);
  pShader->bind(ri, matModel);
  ri.reflectedObjects.pop_back();
}

const std::string& KRMaterial::getName() const
{
  return m_name;
}


bool KRMaterial::getShaderValue(ShaderValue value, float* output) const
{
  switch (value) {
  case ShaderValue::material_alpha:
    *output = m_tr;
    return true;
  case ShaderValue::material_shininess:
    *output = m_ns;
    return true;
  }
  return false;
}

bool KRMaterial::getShaderValue(ShaderValue value, hydra::Vector2* output) const
{
  switch (value) {
  case ShaderValue::diffusetexture_scale:
    *output = m_diffuse.scale;
    return true;
  case ShaderValue::speculartexture_scale:
    *output = m_specular.scale;
    return true;
  case ShaderValue::reflectiontexture_scale:
    *output = m_reflection.scale;
    return true;
  case ShaderValue::normaltexture_scale:
    *output = m_normal.scale;
    return true;
  case ShaderValue::diffusetexture_offset:
    *output = m_diffuse.offset;
    return true;
  case ShaderValue::speculartexture_offset:
    *output = m_specular.offset;
    return true;
  case ShaderValue::reflectiontexture_offset:
    *output = m_reflection.offset;
    return true;
  case ShaderValue::normaltexture_offset:
    *output = m_normal.offset;
    return true;
  }
  return false;
}

bool KRMaterial::getShaderValue(ShaderValue value, hydra::Vector3* output) const
{
  switch (value) {
  case ShaderValue::material_reflection:
    *output = m_reflectionColor;
    return true;
  case ShaderValue::material_ambient:
    *output = m_ambientColor;
    return true;
  case ShaderValue::material_diffuse:
    *output = m_diffuseColor;
    return true;
  case ShaderValue::material_specular:
    *output = m_specularColor;
    return true;
  }
  return false;
}
