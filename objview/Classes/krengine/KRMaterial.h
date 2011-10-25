//
//  KRMaterial.h
//  gldemo
//
//  Created by Kearwood Gilbert on 10-10-24.
//  Copyright (c) 2010 Kearwood Software. All rights reserved.
//

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <stdint.h>
#import <list>
using std::list;

#ifndef KRMATERIAL_H
#define KRMATRIAL_H

#import "KRTexture.h"

class KRMaterial {
public:
    KRMaterial();
    ~KRMaterial();
    
    void setAmbientMap(KRTexture *pTexture);
    void setDiffuseMap(KRTexture *pTexture);
    void setSpecularMap(KRTexture *pTexture);
    void setNormalMap(KRTexture *pTexture);
    void setAmbient(GLfloat r, GLfloat g, GLfloat b);
    void setDiffuse(GLfloat r, GLfloat g, GLfloat b);    
    void setSpecular(GLfloat r, GLfloat g, GLfloat b);    
    
    void bind(GLuint program);
    
private:
    KRTexture *m_pAmbientMap; // mtl map_Ka value
    KRTexture *m_pDiffuseMap; // mtl map_Kd value
    KRTexture *m_pSpecularMap; // mtl map_Ks value
    KRTexture *m_pNormalMap; // mtl map_Normal value
    
    GLfloat m_ka_r, m_ka_g, m_ka_b; // Ambient rgb
    GLfloat m_kd_r, m_kd_g, m_kd_b; // Diffuse rgb
    GLfloat m_ks_r, m_ks_g, m_ks_b; // Specular rgb
};

#endif
