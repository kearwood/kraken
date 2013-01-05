//
//  FileManager.h
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

#ifndef KRAUDIO_MANAGER_H
#define KRAUDIO_MANAGER_H

#import "KREngine-common.h"

#include "KRAudioSample.h"
#include "KRContextObject.h"
#include "KRDataBlock.h"
#include "KRmat4.h"

#include <map>
#include <set>
#include <string>

using std::map;
using std::set;

const int KRENGINE_AUDIO_MAX_POOL_SIZE = 32;
const int KRENGINE_AUDIO_MAX_BUFFER_SIZE = 64*1024;
const int KRENGINE_AUDIO_BUFFERS_PER_SOURCE = 3;

class KRAudioManager : public KRContextObject {
public:
    KRAudioManager(KRContext &context);
    virtual ~KRAudioManager();
    
    void add(KRAudioSample *Sound);
    
    KRAudioSample *load(const std::string &name, const std::string &extension, KRDataBlock *data);
    KRAudioSample *get(const std::string &name);
    
    void setViewMatrix(const KRMat4 &viewMatrix);
    

    void makeCurrentContext();
    
    KRDataBlock *getBufferData(int size);
    void recycleBufferData(KRDataBlock *data);
    
private:
    map<std::string, KRAudioSample *> m_sounds;
    
    std::vector<KRDataBlock *> m_bufferPoolIdle;
    
    ALCcontext* m_alContext;
    ALCdevice* m_alDevice;

};

#endif /* defined(KRAUDIO_MANAGER_H) */
