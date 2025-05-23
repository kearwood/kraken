//
//  KRShader.h
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
#include "KRContextObject.h"
#include "block.h"
#include "resources/KRResource.h"
#include "spirv_reflect.h"

enum class ShaderStage : uint8_t
{
  vert = 0,
  frag,
  tesc,
  tese,
  geom,
  comp,
  mesh,
  task,
  rgen,
  rint,
  rahit,
  rchit,
  rmiss,
  rcall,
  ShaderStageCount,
  Invalid = 0xff
};

ShaderStage getShaderStageFromExtension(const char* extension);
VkShaderStageFlagBits getShaderStageFlagBitsFromShaderStage(ShaderStage stage);

static const size_t kShaderStageCount = static_cast<size_t>(ShaderStage::ShaderStageCount);

class KRShader : public KRResource
{
public:
  KRShader(KRContext& context, std::string name, std::string extension);
  KRShader(KRContext& context, std::string name, std::string extension, mimir::Block* data);
  virtual ~KRShader();

  virtual std::string getExtension();
  std::string& getSubExtension();

  bool createShaderModule(VkDevice& device, VkShaderModule& module);

  virtual bool save(mimir::Block& data);

  mimir::Block* getData();
  const SpvReflectShaderModule* getReflection();
  ShaderStage getShaderStage() const;
  VkShaderStageFlagBits getShaderStageFlagBits() const;
  

private:

  std::string m_extension;
  std::string m_subExtension;
  mimir::Block* m_pData;
  SpvReflectShaderModule m_reflection;
  bool m_reflectionValid;

  void parseReflection();
  void freeReflection();

  ShaderStage m_stage;
};
