//
//  KRResource+gltf.cpp
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

#include "KRResource.h"
#include "bundle/KRBundle.h"
#include "scene/KRScene.h"

#include "mimir.h"

using namespace mimir;
using namespace hydra;

#include "simdjson.h"
using namespace simdjson;

KRBundle* LoadGltf(KRContext& context, simdjson::ondemand::object& jsonRoot, std::vector<Block>& buffers, const std::string& baseName)
{
  std::string_view version;
  if (!tryJsonRequired(jsonRoot["asset"]["version"].get(version))) {
    return nullptr;
  }
  
  std::string_view minVersion;
  if(tryJson(jsonRoot["asset"]["minVersion"].get(minVersion))) {
    // We have a minVersion field.
    // We currently support only version 2.0
    if (minVersion != "2.0") {
      // TODO - Report and handle error
      return nullptr;
    }
  } else {
    // We don't have a minversion field.
    // We will support any version 2.x.
    if (!version.starts_with("2.")) {
      // TODO - Report and handle error
      return nullptr;
    }
  }
  
  
  struct TextureInfo
  {
    std::string name;
    KRTexture* texture = nullptr;
    int imageIndex = -1;
    hydra::Vector2 scale{ 1.f, 1.f };
    hydra::Vector2 offset{ 0.f, 0.f };
    float rotation{ 0.f };
  };
  std::vector<TextureInfo> textures;
  simdjson::ondemand::array jsonTextures;
  if(tryJson(jsonRoot["textures"].get_array().get(jsonTextures)))
  {
    int textureIndex = 0;
    for (auto jsonTexture : jsonTextures) {
      TextureInfo& texture = textures.emplace_back();
      std::string textureName;
      std::string_view textureNameVal;
      if (tryJson(jsonTexture["name"].get(textureNameVal))) {
        texture.name = textureNameVal;
      } else {
        // Name not found in JSON. Generate a fall-back name.
        texture.name = std::format("{}_texture_{}", baseName, textureIndex);
      }
      
      // texture["sampler"] ...
      tryJson(jsonTexture["source"].get(texture.imageIndex));
      simdjson::ondemand::object extensions;
      if(tryJson(jsonTexture["extensions"].get(extensions))) {
        simdjson::ondemand::object textureTransform;
        if(tryJson(extensions["KHR_texture_transform"].get(textureTransform))) {
          tryJson(textureTransform["offset"].get(texture.offset));
          tryJson(textureTransform["rotation"].get(texture.rotation));
          tryJson(textureTransform["scale"].get(texture.scale));
        }
      }
      textureIndex++;
    }
  }
  
  simdjson::ondemand::array images;
  tryJson(jsonRoot["images"].get_array().get(images));
  
  simdjson::ondemand::array samplers;
  tryJson(jsonRoot["samplers"].get_array().get(samplers));

  KRBundle* bundle = new KRBundle(context, baseName);

  std::vector<KRMaterial*> materials;
  simdjson::ondemand::array jsonMaterials;
  if (tryJson(jsonRoot["materials"].get_array().get(jsonMaterials))) {
    int materialIndex = 0;
    for (auto jsonMaterial : jsonMaterials) {
      std::string materialName;
      std::string_view materialNameVal;
      if (tryJson(jsonMaterial["name"].get(materialNameVal))) {
        materialName = materialNameVal;
      } else {
        // Name not found in JSON. Generate a fall-back name.
        materialName = std::format("{}_material_{}", baseName, materialIndex);
      }
      
      KRMaterial* new_material = new KRMaterial(context, std::string(materialName).c_str());
      simdjson::ondemand::object pbrMetallicRoughnessObj;
      if(tryJson(jsonMaterial["pbrMetallicRoughness"].get(pbrMetallicRoughnessObj))) {
        /*
         
         KRTextureBinding texture;
         int texCoord{ 0 };
         hydra::Vector2 scale{ 1.f, 1.f };
         hydra::Vector2 offset{ 0.f, 0.f };
         float rotation{ 0.f };
         
         */
        
        tryJson(pbrMetallicRoughnessObj["baseColorFactor"].get(new_material->m_baseColorFactor));
        simdjson::ondemand::object baseColorTextureInfo;
        if(tryJson(pbrMetallicRoughnessObj["baseColorTexture"].get(baseColorTextureInfo))) {
          tryJson(baseColorTextureInfo["texCoord"].get(new_material->m_baseColorMap.texCoord));
          int textureIndex = -1;
          if(tryJson(baseColorTextureInfo["index"].get(textureIndex))) {
            if (textureIndex < 0 || textureIndex >= textures.size()) {
              // TODO: Log error...
            } else {
              const TextureInfo& texture = textures[textureIndex];
              new_material->m_baseColorMap.scale = texture.scale;
              new_material->m_baseColorMap.offset = texture.offset;
              new_material->m_baseColorMap.rotation = texture.rotation;
              // new_material->m_baseColorMap.texture = ...
            }
          }
        } // baseColorTexture
        // baseColorTexture...
        tryJson(pbrMetallicRoughnessObj["metallicFactor"].get(new_material->m_metalicFactor));
        tryJson(pbrMetallicRoughnessObj["roughnessFactor"].get(new_material->m_roughnessFactor));
        // metallicRoughnessTexture...

      }
      new_material->moveToBundle(bundle);
      materials.push_back(new_material);
      materialIndex++;
    }
  }

  KRScene* pScene = new KRScene(context, baseName + "_scene");

  context.getSceneManager()->add(pScene);
  KrResult result = pScene->moveToBundle(bundle);
  // TODO - Validate result
  bundle->append(*pScene);

  
  return bundle;
}

KRBundle* KRResource::LoadGltf(KRContext& context, const std::string& path)
{
  std::string filePath = util::GetFilePath(path);
  std::string fileBase = util::GetFileBase(path);
  std::string binFilePath = filePath + fileBase + ".bin";

  Block jsonData;
  Block binData;
  if (!jsonData.load(path)) {
    return nullptr;
  }

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  
  jsonData.expand(SIMDJSON_PADDING);
  jsonData.lock();
  auto error = parser.iterate((const char*)jsonData.getStart(), jsonData.getSize()).get(doc);
  jsonData.unlock();

  if (error) {
    // TODO - Report and handle error
    return;
  }
  
  ondemand::object jsonRoot;
  error = doc.get_object().get(jsonRoot);
  if (error) {
    // TODO - Report and handle error
    return;
  }

  simdjson::ondemand::array jsonBuffers;
  error = jsonRoot["buffers"].get_array().get(jsonBuffers);
  if (error) {
    // TODO - Report and handle error
    return nullptr;
  }

  std::vector<Block> buffers;
  for (auto jsonBuffer : jsonBuffers) {
    std::string_view bufferUri;
    error = jsonBuffer["uri"].get_string().get(bufferUri);
    if (error) {
      // TODO - Report and handle error
      return nullptr;
    }
    Block& block = buffers.emplace_back();
    if (!block.load(std::string(bufferUri))) {
      // TODO - Report and handle error
      return nullptr;
    }
  }

  return ::LoadGltf(context, jsonRoot, buffers, fileBase);
}
