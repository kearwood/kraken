//
//  KRTexture.cpp
//  KREngine
//
//  Created by Kearwood Gilbert on 2012-10-05.
//  Copyright (c) 2012 Kearwood Software. All rights reserved.
//

#include "KREngine-common.h"
#include "KRTexture.h"
#include "KRDataBlock.h"
#include "KRContext.h"
#include "KRTextureManager.h"

KRTexture::KRTexture(KRContext &context, std::string name) : KRResource(context, name)
{
    m_iHandle = 0;
    m_textureMemUsed = 0;
    m_last_frame_used = 0;
}

KRTexture::~KRTexture()
{
    releaseHandle();
}

void KRTexture::releaseHandle() {
    if(m_iHandle != 0) {
        GLDEBUG(glDeleteTextures(1, &m_iHandle));
        getContext().getTextureManager()->memoryChanged(-getMemSize());
        m_iHandle = 0;
        m_textureMemUsed = 0;
    }
}

long KRTexture::getMemSize() {
    return m_textureMemUsed; // TODO - This is not 100% accurate, as loaded format may differ in size while in GPU memory
}

long KRTexture::getReferencedMemSize() {
    // Return the amount of memory used by other textures referenced by this texture (for cube maps and animated textures)
    return 0;
}

void KRTexture::resize(int max_dim)
{
    if(max_dim == 0) {
        releaseHandle();
    } else {
        int target_dim = max_dim;
        if(target_dim < m_min_lod_max_dim) target_dim = m_min_lod_max_dim;
        int requiredMemoryTransfer = getThroughputRequiredForResize(target_dim);
        int requiredMemoryDelta = getMemRequiredForSize(target_dim) - getMemSize() - getReferencedMemSize();
        
        if(requiredMemoryDelta) {
            // Only resize / regenerate the texture if it actually changes the size of the texture (Assumption: textures of different sizes will always consume different amounts of memory)
        
            if(getContext().getTextureManager()->getMemoryTransferedThisFrame() + requiredMemoryTransfer > getContext().KRENGINE_MAX_TEXTURE_THROUGHPUT) {
                // Exceeding per-frame transfer throughput; can't resize now
                return;
            }
            
            if(getContext().getTextureManager()->getMemUsed() + requiredMemoryDelta > getContext().KRENGINE_MAX_TEXTURE_MEM) {
                // Exceeding total memory allocated to textures; can't resize now
                return;
            }
            
            if(m_current_lod_max_dim != target_dim || m_iHandle == 0) {
                releaseHandle();
            }
            if(m_iHandle == 0) {
                if(!createGLTexture(target_dim)) {
                    assert(false);
                }
            }
        }
    }
}


GLuint KRTexture::getHandle() {
    if(m_iHandle == 0) {
        //resize(getContext().KRENGINE_MIN_TEXTURE_DIM);
        resize(m_min_lod_max_dim);
    }
    resetPoolExpiry();
    return m_iHandle;
}

void KRTexture::resetPoolExpiry()
{
    m_last_frame_used = getContext().getCurrentFrame();
}

long KRTexture::getThroughputRequiredForResize(int max_dim)
{
    // Calculate the throughput required for GPU texture upload if the texture is resized to max_dim.
    // This default behaviour assumes that the texture will need to be deleted and regenerated to change the maximum mip-map level.
    // If an OpenGL extension is present that allows a texture to be resized incrementally, then this method should be overridden
    
    if(max_dim == 0) {
        return 0;
    } else {    
        int target_dim = max_dim;
        if(target_dim < m_min_lod_max_dim) target_dim = target_dim;
        
        
        if(target_dim != m_current_lod_max_dim) {
            int requiredMemory = getMemRequiredForSize(target_dim);
            int requiredMemoryDelta = requiredMemory - getMemSize() - getReferencedMemSize();
            
            if(requiredMemoryDelta == 0) {
                // Only resize / regenerate the texture if it actually changes the size of the texture (Assumption: textures of different sizes will always consume different amounts of memory)
                return 0;
            }
            return requiredMemory;
        } else {
            return 0;
        }
    }
}

long KRTexture::getLastFrameUsed()
{
    return m_last_frame_used;
}

bool KRTexture::isAnimated()
{
    return false;
}

KRTexture *KRTexture::compress()
{
    return NULL;
}
