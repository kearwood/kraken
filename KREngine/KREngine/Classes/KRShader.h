//
//  KRShader.h
//  KREngine
//
//  Copyright 2012 Kearwood Gilbert. All rights reserved.
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

#import <stdint.h>
#import <vector>
#import <string>

#import "KREngine-common.h"

using std::vector;

#ifndef KRSHADER_H
#define KRSHADER_H

#import "KRShader.h"
#import "KRMat4.h"
#import "KRCamera.h"

class KRShader {
public:
    KRShader(char *szKey, std::string options, const GLchar *szVertShaderSource, const GLchar *szFragShaderSource);
    ~KRShader();
    GLuint getProgram();
    char *getKey();
    
    void bind(KRCamera *pCamera, KRMat4 &mvpMatrix, KRVector3 &cameraPosition, KRVector3 &lightDirection, KRMat4 *pShadowMatrices, GLuint *shadowDepthTextures, int cShadowBuffers);
    
    enum {
        KRENGINE_ATTRIB_VERTEX,
        KRENGINE_ATTRIB_NORMAL,
        KRENGINE_ATTRIB_TANGENT,
        KRENGINE_ATTRIB_TEXUVA,
        KRENGINE_ATTRIB_TEXUVB,
        KRENGINE_NUM_ATTRIBUTES
    };
    
    enum {
        KRENGINE_UNIFORM_MATERIAL_AMBIENT,
        KRENGINE_UNIFORM_MATERIAL_DIFFUSE,
        KRENGINE_UNIFORM_MATERIAL_SPECULAR,
        KRENGINE_UNIFORM_MATERIAL_ALPHA,
        KRENGINE_UNIFORM_MATERIAL_SHININESS,
        KRENGINE_UNIFORM_MVP,
        KRENGINE_UNIFORM_LIGHTDIRECTION,
        KRENGINE_UNIFORM_CAMERAPOS,
        KRENGINE_UNIFORM_DIFFUSETEXTURE,
        KRENGINE_UNIFORM_SPECULARTEXTURE,
        KRENGINE_UNIFORM_NORMALTEXTURE,
        KRENGINE_UNIFORM_DIFFUSETEXTURE_SCALE,
        KRENGINE_UNIFORM_SPECULARTEXTURE_SCALE,
        KRENGINE_UNIFORM_NORMALTEXTURE_SCALE,
        KRENGINE_UNIFORM_AMBIENTTEXTURE_SCALE,
        KRENGINE_UNIFORM_DIFFUSETEXTURE_OFFSET,
        KRENGINE_UNIFORM_SPECULARTEXTURE_OFFSET,
        KRENGINE_UNIFORM_NORMALTEXTURE_OFFSET,
        KRENGINE_UNIFORM_AMBIENTTEXTURE_OFFSET,
        KRENGINE_UNIFORM_SHADOWMVP1,
        KRENGINE_UNIFORM_SHADOWMVP2,
        KRENGINE_UNIFORM_SHADOWMVP3,
        KRENGINE_UNIFORM_SHADOWTEXTURE1,
        KRENGINE_UNIFORM_SHADOWTEXTURE2,
        KRENGINE_UNIFORM_SHADOWTEXTURE3,
        
        KRENGINE_NUM_UNIFORMS
    };
    GLint m_uniforms[KRENGINE_NUM_UNIFORMS];
    char m_szKey[128];
    
private:
    GLuint m_iProgram;
};

#endif