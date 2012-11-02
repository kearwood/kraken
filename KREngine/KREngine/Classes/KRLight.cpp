//
//  KRLight.cpp
//  KREngine
//
//  Created by Kearwood Gilbert on 12-04-05.
//  Copyright (c) 2012 Kearwood Software. All rights reserved.
//

#include <iostream>



#import "KRLight.h"

#import "KRNode.h"
#import "KRMat4.h"
#import "KRVector3.h"
#import "KRCamera.h"
#import "KRContext.h"

#import "KRShaderManager.h"
#import "KRShader.h"
#import "KRStockGeometry.h"
#import "assert.h"

KRLight::KRLight(KRScene &scene, std::string name) : KRNode(scene, name)
{
    m_intensity = 1.0f;
    m_flareTexture = "";
    m_pFlareTexture = NULL;
    m_flareSize = 0.0;
}

KRLight::~KRLight()
{

}

tinyxml2::XMLElement *KRLight::saveXML( tinyxml2::XMLNode *parent)
{
    tinyxml2::XMLElement *e = KRNode::saveXML(parent);
    e->SetAttribute("intensity", m_intensity);
    e->SetAttribute("color_r", m_color.x);
    e->SetAttribute("color_g", m_color.y);
    e->SetAttribute("color_b", m_color.z);
    e->SetAttribute("decay_start", m_decayStart);
    e->SetAttribute("flare_size", m_flareSize);
    e->SetAttribute("flare_texture", m_flareTexture.c_str());
    return e;
}

void KRLight::loadXML(tinyxml2::XMLElement *e) {
    KRNode::loadXML(e);
    float x,y,z;
    if(e->QueryFloatAttribute("color_r", &x) != tinyxml2::XML_SUCCESS) {
        x = 1.0;
    }
    if(e->QueryFloatAttribute("color_g", &y) != tinyxml2::XML_SUCCESS) {
        y = 1.0;
    }
    if(e->QueryFloatAttribute("color_b", &z) != tinyxml2::XML_SUCCESS) {
        z = 1.0;
    }
    m_color = KRVector3(x,y,z);
    
    if(e->QueryFloatAttribute("intensity", &m_intensity) != tinyxml2::XML_SUCCESS) {
        m_intensity = 100.0;
    }
    
    if(e->QueryFloatAttribute("decay_start", &m_decayStart) != tinyxml2::XML_SUCCESS) {
        m_decayStart = 0.0;
    }
    
    if(e->QueryFloatAttribute("flare_size", &m_flareSize) != tinyxml2::XML_SUCCESS) {
        m_flareSize = 0.0;
    }
    
    const char *szFlareTexture = e->Attribute("flare_texture");
    if(szFlareTexture) {
        m_flareTexture = szFlareTexture;
    } else {
        m_flareTexture = "";
    }
    m_pFlareTexture = NULL;
}

void KRLight::setFlareTexture(std::string flare_texture) {
    m_flareTexture = flare_texture;
    m_pFlareTexture = NULL;
}

void KRLight::setFlareSize(float flare_size) {
    m_flareSize = flare_size;
}

void KRLight::setIntensity(float intensity) {
    m_intensity = intensity;
}
float KRLight::getIntensity() {
    return m_intensity;
}

const KRVector3 &KRLight::getColor() {
    return m_color;
}

void KRLight::setColor(const KRVector3 &color) {
    m_color = color;
}

void KRLight::setDecayStart(float decayStart) {
    m_decayStart = decayStart;
}

float KRLight::getDecayStart() {
    return m_decayStart;
}

#if TARGET_OS_IPHONE

void KRLight::render(KRCamera *pCamera, KRContext *pContext, const KRViewport &viewport, KRVector3 &lightDirection, KRMat4 *pShadowMatrices, GLuint *shadowDepthTextures, int cShadowBuffers, KRNode::RenderPass renderPass) {

    KRNode::render(pCamera, pContext, viewport, lightDirection, pShadowMatrices, shadowDepthTextures, cShadowBuffers, renderPass);
    
    if(renderPass == KRNode::RENDER_PASS_ADDITIVE_PARTICLES) {
        if(m_flareTexture.size() && m_flareSize > 0.0f) {
            if(!m_pFlareTexture && m_flareTexture.size()) {
                m_pFlareTexture = pContext->getTextureManager()->getTexture(m_flareTexture.c_str());
            }
            
            if(m_pFlareTexture) {
                
                KRVector3 light_position = getLocalTranslation();
                
                KRMat4 m_modelMatrix = KRMat4();
                m_modelMatrix.translate(light_position.x, light_position.y, light_position.z);
                
                // Disable z-buffer test
                GLDEBUG(glDisable(GL_DEPTH_TEST));
                GLDEBUG(glDepthRangef(0.0, 1.0));
                
                // Render light flare on transparency pass
                KRShader *pShader = pContext->getShaderManager()->getShader("flare", pCamera, false, false, false, 0, false, false, false, false, false, false, false, false, false, false, false, false, false, renderPass);
                if(pShader->bind(viewport, m_modelMatrix, lightDirection, pShadowMatrices, shadowDepthTextures, 0, renderPass)) {
                    GLDEBUG(glUniform1f(
                                        pShader->m_uniforms[KRShader::KRENGINE_UNIFORM_FLARE_SIZE],
                                        m_flareSize
                                        ));
                    m_pContext->getTextureManager()->selectTexture(0, m_pFlareTexture, 2048);
                    m_pContext->getModelManager()->bindVBO((void *)KRENGINE_VBO_2D_SQUARE, KRENGINE_VBO_2D_SQUARE_SIZE, true, false, false, true, false);
                    GLDEBUG(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
                }
            }
        }
        
    }
}

#endif
