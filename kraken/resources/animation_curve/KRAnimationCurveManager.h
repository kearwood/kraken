//
//  KRAnimationCurveManager.h
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

#include "resources/KRResourceManager.h"

#include "KRAnimationCurve.h"
#include "KRContextObject.h"
#include "block.h"

using std::map;

class KRAnimationCurveManager : public KRResourceManager
{
public:
  KRAnimationCurveManager(KRContext& context);
  virtual ~KRAnimationCurveManager();

  virtual KRResource* loadResource(const std::string& name, const std::string& extension, mimir::Block* data) override;
  virtual KRResource* getResource(const std::string& name, const std::string& extension) override;

  KRAnimationCurve* loadAnimationCurve(const std::string& name, mimir::Block* data);
  KRAnimationCurve* getAnimationCurve(const std::string& name);
  void addAnimationCurve(KRAnimationCurve* new_animation_curve);
  unordered_map<std::string, KRAnimationCurve*>& getAnimationCurves();

  void deleteAnimationCurve(KRAnimationCurve* curve);

private:
  unordered_map<std::string, KRAnimationCurve*> m_animationCurves;
};

