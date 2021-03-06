//
//  KRBone.h
//  KREngine
//
//  Created by Kearwood Gilbert on 2012-12-06.
//  Copyright (c) 2012 Kearwood Software. All rights reserved.
//

#ifndef KRBONE_H
#define KRBONE_H

#include "KRResource.h"
#include "KRNode.h"
#include "KRTexture.h"

class KRBone : public KRNode {
public:
    static void InitNodeInfo(KrNodeInfo* nodeInfo);

    KRBone(KRScene &scene, std::string name);
    virtual ~KRBone();
    virtual std::string getElementName();
    virtual tinyxml2::XMLElement *saveXML( tinyxml2::XMLNode *parent);
    virtual void loadXML(tinyxml2::XMLElement *e);
    virtual AABB getBounds();
    
    void render(KRCamera *pCamera, std::vector<KRPointLight *> &point_lights, std::vector<KRDirectionalLight *> &directional_lights, std::vector<KRSpotLight *>&spot_lights, const KRViewport &viewport, KRNode::RenderPass renderPass);

    void setBindPose(const Matrix4 &pose);
    const Matrix4 &getBindPose();
private:
    Matrix4 m_bind_pose;
};


#endif
