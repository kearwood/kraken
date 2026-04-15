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
#include "simdjson.h"

using namespace mimir;
using namespace hydra;
using namespace simdjson;

namespace simdjson {

template <typename builder_type>
void tag_invoke(serialize_tag, builder_type& builder, const hydra::Vector2& vec)
{
  builder.start_array();
  builder.append(vec.x);
  builder.append_comma();
  builder.append(vec.y);
  builder.end_array();
}

template <typename builder_type>
void tag_invoke(serialize_tag, builder_type& builder, const hydra::Vector3& vec)
{
  builder.start_array();
  builder.append(vec.x);
  builder.append_comma();
  builder.append(vec.y);
  builder.append_comma();
  builder.append(vec.z);
  builder.end_array();
}

template <typename builder_type>
void tag_invoke(serialize_tag, builder_type& builder, const hydra::Vector4& vec)
{
  builder.start_array();
  builder.append(vec.x);
  builder.append_comma();
  builder.append(vec.y);
  builder.append_comma();
  builder.append(vec.z);
  builder.append_comma();
  builder.append(vec.w);
  builder.end_array();
}

template <typename builder_type>
void tag_invoke(serialize_tag, builder_type& builder, const KRMaterial::TransformedTexture& texture)
{
  builder.start_object();
  builder.append_key_value<"texture">(texture.texture.getName());
  builder.append_comma();
  builder.append_key_value<"offset">(texture.offset);
  builder.append_comma();
  builder.append_key_value<"scale">(texture.scale);
  builder.append_comma();
  builder.append_key_value<"rotation">(texture.rotation);
  builder.end_object();
}

} // namespace simdjson

KRMaterial::KRMaterial(KRContext& context, const char* name)
  : KRResource(context, name)
{
}

KRMaterial::KRMaterial(KRContext& context, std::string name, mimir::Block* data)
  : KRResource(context, name)
{
  simdjson::dom::parser parser;
  simdjson::dom::element jsonRoot;
  data->lock();

  /*
  char* str = (char*)data->getStart();
  OutputDebugStringA("\n\n----====----\n");
  OutputDebugStringA(str);
  OutputDebugStringA("\n----====----\n\n");
  */

  auto error = parser.parse((const char*)data->getStart(), data->getSize()).get(jsonRoot);
  data->unlock();
  if (error) {
    // TODO - Report and handle error
  }
}

KRMaterial::~KRMaterial()
{

}

std::string KRMaterial::getExtension()
{
  return "krmaterial";
}

bool KRMaterial::needsVertexTangents()
{
  return m_normalTexture.texture.isSet();
}

bool KRMaterial::save(Block& data)
{
  simdjson::builder::string_builder sb;
  sb.start_object();
  sb.append_key_value<"name">(getName());
  sb.append_comma();
  switch (m_alphaMode) {
  case KRMATERIAL_ALPHA_MODE_OPAQUE:
    sb.append_key_value<"alphaMode">("opaque");
    break;
  case KRMATERIAL_ALPHA_MODE_BLEND:
    sb.append_key_value<"alphaMode">("blend");
    break;
  case KRMATERIAL_ALPHA_MODE_TEST:
    sb.append_key_value<"alphaMode">("test");
    break;
  }
  sb.append_comma();
  sb.append_key_value<"alphaCutoff">(m_alphaCutoff);
  sb.append_comma();
  sb.append_key_value<"ior">(m_ior);
  sb.append_comma();
  sb.append_key_value<"dispersion">(m_dispersion);
  sb.append_comma();
  switch (m_shadingModel) {
  case KRMATERIAL_SHADING_MODEL_UNLIT:
    sb.append_key_value<"shadingModel">("unlit");
    break;
  case KRMATERIAL_SHADING_MODEL_PBR:
    sb.append_key_value<"shadingModel">("pbr");
    break;
  }

  sb.append_comma();
  sb.escape_and_append_with_quotes("baseColor");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(m_baseColorTexture);
  sb.append_comma();
  sb.append_key_value<"factor">(m_baseColorFactor);
  sb.end_object();

  sb.append_comma();

  sb.escape_and_append_with_quotes("normal");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(m_normalTexture);
  sb.append_comma();
  sb.append_key_value<"scale">(m_normalScale);
  sb.end_object();

  sb.append_comma();

  sb.escape_and_append_with_quotes("emissive");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(m_emissiveTexture);
  sb.append_comma();
  sb.append_key_value<"factor">(m_emissiveFactor);
  sb.end_object();


  sb.append_comma();


  sb.escape_and_append_with_quotes("occlusion");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(m_occlusionTexture);
  sb.append_comma();
  sb.append_key_value<"strength">(m_occlusionStrength);
  sb.end_object();

  sb.append_comma();

  sb.escape_and_append_with_quotes("metalicRoughness");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(m_metalicRoughness);
  sb.append_comma();
  sb.append_key_value<"metalicFactor">(m_metalicFactor);
  sb.append_comma();
  sb.append_key_value<"roughnessFactor">(m_roughnessFactor);
  sb.end_object();

  sb.append_comma();

  sb.escape_and_append_with_quotes("anisotropy");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(m_anisotropyTexture);
  sb.append_comma();
  sb.append_key_value<"strength">(m_anisotropyStrength);
  sb.append_comma();
  sb.append_key_value<"rotation">(m_anisotropyRotation);
  sb.end_object();

  sb.append_comma();

  sb.escape_and_append_with_quotes("clearcoat");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(m_clearcoatTexture);
  sb.append_comma();
  sb.append_key_value<"factor">(m_clearcoatFactor);
  sb.append_comma();
  sb.append_key_value<"roughnessMap">(m_clearcoatTexture);
  sb.append_comma();
  sb.append_key_value<"roughnessFactor">(m_anisotropyRotation);
  sb.end_object();

  sb.append_comma();

  sb.escape_and_append_with_quotes("specular");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(m_specularTexture);
  sb.append_comma();
  sb.append_key_value<"factor">(m_specularFactor);
  sb.append_comma();
  sb.append_key_value<"colorMap">(m_specularColorTexture);
  sb.append_comma();
  sb.append_key_value<"colorFactor">(m_specularColorFactor);
  sb.end_object();

  sb.append_comma();

  sb.escape_and_append_with_quotes("thickness");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(m_thicknessTexture);
  sb.append_comma();
  sb.append_key_value<"factor">(m_thicknessFactor);
  sb.append_comma();
  sb.append_key_value<"attenuiationDistance">(m_attenuationDistance);
  sb.append_comma();
  sb.append_key_value<"attenuationColor">(m_attenuationColor);
  sb.end_object();


  sb.append_comma();

  sb.escape_and_append_with_quotes("transmission");
  sb.append_colon();

  sb.start_object();
  sb.append_key_value<"map">(transmissionTexture);
  sb.append_comma();
  sb.append_key_value<"factor">(transmissionFactor);
  sb.end_object();


  sb.end_object();


  const char* str = nullptr;
  auto error = sb.c_str().get(str);
  if (error) {
    return false;
    // TODO - Report error
  }
  data.append(str);

  return true;
  /*

  std::stringstream stream;
  stream.precision(std::numeric_limits<long double>::digits10);
  stream.setf(std::ios::fixed, std::ios::floatfield);

  stream << "newmtl " << getName();
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
  switch (m_alphaMode) {
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
  */
}

/*
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
*/

void KRMaterial::setAlphaMode(KRMaterial::alpha_mode_type alpha_mode)
{
  m_alphaMode = alpha_mode;
}

KRMaterial::alpha_mode_type KRMaterial::getAlphaMode()
{
  return m_alphaMode;
}
/*
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
*/

void KRMaterial::setTransparency(float a)
{
  if (a < 1.0f && m_alphaMode == KRMaterial::KRMATERIAL_ALPHA_MODE_OPAQUE) {
    setAlphaMode(KRMaterial::KRMATERIAL_ALPHA_MODE_BLEND);
  }
  m_baseColorFactor[3] = a;
}

void KRMaterial::setShininess(float s)
{
  m_roughnessFactor = 1.0f - s;
}

bool KRMaterial::isTransparent()
{
  return m_baseColorFactor[3] < 1.0 || m_alphaMode == KRMATERIAL_ALPHA_MODE_BLEND;
}

void KRMaterial::getResourceBindings(std::list<KRResourceBinding*>& bindings)
{
  KRResource::getResourceBindings(bindings);

  bindings.push_back(&m_baseColorTexture.texture);
  bindings.push_back(&m_normalTexture.texture);
  bindings.push_back(&m_emissiveTexture.texture);
  bindings.push_back(&m_occlusionTexture.texture);
  bindings.push_back(&m_metalicRoughness.texture);
  bindings.push_back(&m_anisotropyTexture.texture);
  bindings.push_back(&m_clearcoatTexture.texture);
  bindings.push_back(&m_clearcoatRoughnessTexture.texture);
  bindings.push_back(&m_clearcoatNormalTexture.texture);
  bindings.push_back(&m_specularTexture.texture);
  bindings.push_back(&m_specularColorTexture.texture);
  bindings.push_back(&m_thicknessTexture.texture);
}

kraken_stream_level KRMaterial::getStreamLevel()
{
  kraken_stream_level stream_level = kraken_stream_level::STREAM_LEVEL_IN_HQ;

  if (m_baseColorTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_baseColorTexture.texture.get()->getStreamLevel());
  }

  if (m_normalTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_normalTexture.texture.get()->getStreamLevel());
  }

  if (m_occlusionTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_occlusionTexture.texture.get()->getStreamLevel());
  }

  if (m_metalicRoughness.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_metalicRoughness.texture.get()->getStreamLevel());
  }

  if (m_anisotropyTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_anisotropyTexture.texture.get()->getStreamLevel());
  }

  if (m_clearcoatTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_clearcoatTexture.texture.get()->getStreamLevel());
  }

  if (m_clearcoatTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_clearcoatTexture.texture.get()->getStreamLevel());
  }

  if (m_clearcoatRoughnessTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_clearcoatRoughnessTexture.texture.get()->getStreamLevel());
  }

  if (m_clearcoatNormalTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_clearcoatNormalTexture.texture.get()->getStreamLevel());
  }

  if (m_specularTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_specularTexture.texture.get()->getStreamLevel());
  }

  if (m_specularColorTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_specularColorTexture.texture.get()->getStreamLevel());
  }

  if (m_thicknessTexture.texture.isBound()) {
    stream_level = KRMIN(stream_level, m_thicknessTexture.texture.get()->getStreamLevel());
  }

  return stream_level;
}

void KRMaterial::bind(KRNode::RenderInfo& ri, ModelFormat modelFormat, __uint32_t vertexAttributes, CullMode cullMode, const std::vector<KRBone*>& bones, const std::vector<Matrix4>& bind_poses, const Matrix4& matModel, KRTexture* pLightMap, float lod_coverage)
{
  bool bLightMap = pLightMap && ri.camera->settings.bEnableLightMap;

  Vector2 default_scale = Vector2::One();
  Vector2 default_offset = Vector2::Zero();

  bool bHasReflection = m_roughnessFactor > 0.f;
  bool bDiffuseMap = m_baseColorTexture.texture.isBound() && ri.camera->settings.bEnableDiffuseMap;
  bool bNormalMap = m_normalTexture.texture.isBound() && ri.camera->settings.bEnableNormalMap;
  bool bSpecMap = false;
  bool bReflectionMap = false;
  bool bReflectionCubeMap = false;
  bool bAlphaTest = m_alphaMode == KRMATERIAL_ALPHA_MODE_TEST;
  bool bAlphaBlend = m_alphaMode == KRMATERIAL_ALPHA_MODE_BLEND;

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
  info.bDiffuseMapScale = m_baseColorTexture.scale != default_scale && bDiffuseMap;
  info.bSpecMapScale = false;
  info.bNormalMapScale = m_normalTexture.scale != default_scale && bNormalMap;
  info.bReflectionMapScale = false;
  info.bDiffuseMapOffset = false;
  info.bSpecMapOffset = false;
  info.bNormalMapOffset = m_normalTexture.offset != default_offset && bNormalMap;
  info.bReflectionMapOffset = false;
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
    pShader->setImageBinding("diffuseTexture", m_baseColorTexture.texture.get(), getContext().getSamplerManager()->DEFAULT_WRAPPING_SAMPLER);
  }

  if (bSpecMap) {
    pShader->setImageBinding("specularTexture", m_specularColorTexture.texture.get(), getContext().getSamplerManager()->DEFAULT_WRAPPING_SAMPLER);
  }

  if (bNormalMap) {
    pShader->setImageBinding("normalTexture", m_normalTexture.texture.get(), getContext().getSamplerManager()->DEFAULT_WRAPPING_SAMPLER);
  }

  if (bReflectionCubeMap) {
    // Deprecated by reflection cubes..
    // pShader->setImageBinding("reflectionCubeTexture", m_reflectionCube.get(), getContext().getSamplerManager()->DEFAULT_CLAMPED_SAMPLER);
  }

  if (bReflectionMap) {
    // Deprecated by PBR model..
    // pShader->setImageBinding("reflectionTexture", m_reflection.texture.get(), getContext().getSamplerManager()->DEFAULT_CLAMPED_SAMPLER);
  }

  ri.reflectedObjects.push_back(this);
  pShader->bind(ri, matModel);
  ri.reflectedObjects.pop_back();
}

bool KRMaterial::getShaderValue(ShaderValue value, float* output) const
{
  switch (value) {
  case ShaderValue::material_alpha:
    *output = m_baseColorFactor[3];
    return true;
  case ShaderValue::material_shininess:
    *output = 1.0f - m_roughnessFactor;
    return true;
  }
  return false;
}

bool KRMaterial::getShaderValue(ShaderValue value, hydra::Vector2* output) const
{
  switch (value) {
  case ShaderValue::diffusetexture_scale:
    *output = m_baseColorTexture.scale;
    return true;
  case ShaderValue::speculartexture_scale:
    *output = m_specularColorTexture.scale;
    return true;
  case ShaderValue::normaltexture_scale:
    *output = m_normalTexture.scale;
    return true;
  case ShaderValue::diffusetexture_offset:
    *output = m_baseColorTexture.offset;
    return true;
  case ShaderValue::speculartexture_offset:
    *output = m_specularColorTexture.offset;
    return true;
  case ShaderValue::normaltexture_offset:
    *output = m_normalTexture.offset;
    return true;
  }
  return false;
}

bool KRMaterial::getShaderValue(ShaderValue value, hydra::Vector3* output) const
{
  switch (value) {
  case ShaderValue::material_diffuse:
    *output = hydra::Vector3::Create(m_baseColorFactor);
    return true;
  case ShaderValue::material_specular:
    *output = m_specularColorFactor;
    return true;
  }
  return false;
}
