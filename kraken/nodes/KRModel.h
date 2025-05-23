//
//  KRModel.h
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

#pragma once

#include "KREngine-common.h"

#include "resources/mesh/KRMesh.h"
#include "KRModel.h"
#include "KRCamera.h"
#include "resources/mesh/KRMeshManager.h"
#include "KRNode.h"
#include "KRContext.h"
#include "resources/mesh/KRMesh.h"
#include "resources/texture/KRTexture.h"
#include "KRBone.h"

class KRModel : public KRNode
{

public:
  static void InitNodeInfo(KrNodeInfo* nodeInfo);

  KRModel(KRScene& scene, std::string name);
  KRModel(KRScene& scene, std::string instance_name, std::string model_name, std::string light_map, float lod_min_coverage, bool receives_shadow, bool faces_camera, hydra::Vector3 rim_color = hydra::Vector3::Zero(), float rim_power = 0.0f);
  virtual ~KRModel();
  
  KrResult update(const KrNodeInfo* nodeInfo) override;

  virtual std::string getElementName();
  virtual tinyxml2::XMLElement* saveXML(tinyxml2::XMLNode* parent);

  virtual void render(KRNode::RenderInfo& ri) override;

  virtual hydra::AABB getBounds();

  void setRimColor(const hydra::Vector3& rim_color);
  void setRimPower(float rim_power);
  hydra::Vector3 getRimColor();
  float getRimPower();

  void setLightMap(const std::string& name);
  std::string getLightMap();

  virtual kraken_stream_level getStreamLevel(const KRViewport& viewport);

private:
  void preStream(const KRViewport& viewport);

  std::vector<KRMesh*> m_models;
  unordered_map<KRMesh*, std::vector<KRBone*> > m_bones; // Outer std::map connects model to set of bones
  KRTexture* m_pLightMap;
  std::string m_lightMap;
  std::string m_model_name;


  float m_min_lod_coverage;
  void loadModel();

  bool m_receivesShadow;
  bool m_faces_camera;


  hydra::Matrix4 m_boundsCachedMat;
  hydra::AABB m_boundsCached;


  hydra::Vector3 m_rim_color;
  float m_rim_power;
};
