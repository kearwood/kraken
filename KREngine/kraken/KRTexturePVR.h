//
//  KRTexturePVR.h
//  KREngine
//
//  Created by Kearwood Gilbert on 2012-10-23.
//  Copyright (c) 2012 Kearwood Software. All rights reserved.
//

#ifndef KRTEXTUREPVR_H
#define KRTEXTUREPVR_H

#include "KRTexture2D.h"

class KRTexturePVR : public KRTexture2D
{
public:
    KRTexturePVR(KRContext &context, KRDataBlock *data, std::string name);
    virtual ~KRTexturePVR();
    virtual std::string getExtension();
    
    bool uploadTexture(GLenum target, int lod_max_dim, int &current_lod_max_dim, int prev_lod_max_dim);
    
    virtual long getMemRequiredForSize(int max_dim);
    
protected:
    
    uint32_t  m_iWidth;
    uint32_t  m_iHeight;
    GLenum    m_internalFormat;
    bool      m_bHasAlpha;
    
    std::list<KRDataBlock *> m_blocks;
};

#endif
