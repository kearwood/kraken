//
//  ShaderManager.h
//  KREngine
//
//  Copyright 2019 Kearwood Gilbert. All rights reserved.
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

#ifndef KRSHADER_MANAGER_H
#define KRSHADER_MANAGER_H

#include "KREngine-common.h"

#include "KRResourceManager.h"

#include "KRShader.h"
#include "KRContextObject.h"
#include "KRDataBlock.h"

class KRUnknown;

class KRShaderManager : public KRResourceManager {
public:
    KRShaderManager(KRContext &context);
    virtual ~KRShaderManager();

    virtual KRResource* loadResource(const std::string& name, const std::string& extension, KRDataBlock* data) override;
    virtual KRResource* getResource(const std::string& name, const std::string& extension) override;
    
    void add(KRShader *shader);
    
    KRShader *load(const std::string &name, const std::string &extension, KRDataBlock *data);
    KRShader *get(const std::string &name, const std::string &extension);

    bool compileAll(KRBundle* outputBundle, KRUnknown* logResource);

    const unordered_map<std::string, KRShader *> &get(const std::string &extension);
    
    unordered_map<std::string, unordered_map<std::string, KRShader *> > &getShaders();
    
private:
    unordered_map<std::string, unordered_map<std::string, KRShader *> > m_shaders;
    bool m_initializedGlslang;
};

#endif /* defined(KRUNKNOWN_MANAGER_H) */
