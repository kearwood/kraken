//
//  KRPointLight.h
//  KREngine
//
//  Created by Kearwood Gilbert on 12-04-05.
//  Copyright (c) 2012 Kearwood Software. All rights reserved.
//

#ifndef KRPOINTLIGHT_H
#define KRPOINTLIGHT_H

#include "KRLight.h"
#include "KRMat4.h"

class KRPointLight : public KRLight {
    
public:
    
    KRPointLight(KRScene &scene, std::string name);
    virtual ~KRPointLight();
    
    virtual std::string getElementName();
    virtual KRAABB getBounds();

    virtual void render(KRCamera *pCamera, std::vector<KRLight *> &lights, const KRViewport &viewport, KRNode::RenderPass renderPass);
    
private:
    void generateMesh();
    
    GLfloat *m_sphereVertices;
    int m_cVertices;
};

#endif
