//
//  KRParticleSystemBrownian.h
//  KREngine
//
//  Created by Kearwood Gilbert on 2012-11-02.
//  Copyright (c) 2012 Kearwood Software. All rights reserved.
//

#ifndef KRPARTICLESYSTEMBROWNIAN_H
#define KRPARTICLESYSTEMBROWNIAN_H

#import "KRParticleSystem.h"

class KRParticleSystemBrownian : public KRParticleSystem {
public:
    KRParticleSystemBrownian(KRScene &scene, std::string name);
    virtual ~KRParticleSystemBrownian();
    
    virtual std::string getElementName();
    virtual void loadXML(tinyxml2::XMLElement *e);
    virtual tinyxml2::XMLElement *saveXML( tinyxml2::XMLNode *parent);
private:
    
};

#endif