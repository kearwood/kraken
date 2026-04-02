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

KRBundle* LoadGltf(KRContext& context, const Block& jsonData, const Block& binData, const std::string& baseName)
{
  KRScene* pScene = new KRScene(context, baseName + "_scene");
  KRBundle* bundle = new KRBundle(context, baseName);

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
  if (!binData.load(binFilePath)) {
    return nullptr;
  }

  return ::LoadGltf(context, jsonData, binData, fileBase);
}
