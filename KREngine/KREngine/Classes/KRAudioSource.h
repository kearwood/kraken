//
//  KRAudioSource.h
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

#ifndef KRAUDIOSOURCE_H
#define KRAUDIOSOURCE_H

#import "KRResource.h"
#import "KRNode.h"
#import "KRTexture.h"

#include <queue>

class KRAudioSample;
class KRAudioBuffer;

class KRAudioSource : public KRNode {
public:
    KRAudioSource(KRScene &scene, std::string name);
    virtual ~KRAudioSource();
    virtual std::string getElementName();
    virtual tinyxml2::XMLElement *saveXML( tinyxml2::XMLNode *parent);
    virtual void loadXML(tinyxml2::XMLElement *e);
    virtual bool hasPhysics();
    virtual void physicsUpdate(float deltaTime);
    
    void render(KRCamera *pCamera, std::vector<KRLight *> &lights, const KRViewport &viewport, KRNode::RenderPass renderPass);
    void play();
    void stop();
    bool isPlaying();
    
    void setSample(const std::string &sound_name);
    std::string getSample();
    
    float getGain();
    void setGain(float gain);
    
    float getPitch();
    void setPitch(float pitch);
    
    bool getLooping();
    void setLooping(bool looping);
    
private:
    std::string m_audio_sample_name;
    
    KRAudioSample *m_audioFile;
    unsigned int m_sourceID;
    float m_gain;
    float m_pitch;
    bool m_looping;
    std::queue<KRAudioBuffer *> m_audioBuffers;
    int m_nextBufferIndex;
    bool m_playing;
    bool m_is3d;
    bool m_isPrimed;
    
    void prime();
    void queueBuffer();
};

#endif /* defined(KRAUDIOSOURCE_H) */