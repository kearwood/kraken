//
//  KRBoundingVolume.cpp
//  KREngine
//
//  Created by Kearwood Gilbert on 11-09-29.
//  Copyright 2011 Kearwood Software. All rights reserved.
//

#include <iostream>

#import "KRBoundingVolume.h"


KRBoundingVolume::KRBoundingVolume(const Vector3 *pVertices) {
    for(int iVertex=0; iVertex < 8; iVertex++) {
        m_vertices[iVertex] = pVertices[iVertex];
    }
}

KRBoundingVolume::KRBoundingVolume(const Vector3 &corner1, const Vector3 &corner2, const KRMat4 modelMatrix) {
    m_vertices[0] = Vector3(corner1.x, corner1.y, corner1.z);
    m_vertices[1] = Vector3(corner2.x, corner1.y, corner1.z);
    m_vertices[2] = Vector3(corner2.x, corner2.y, corner1.z);
    m_vertices[3] = Vector3(corner1.x, corner2.y, corner1.z);
    m_vertices[4] = Vector3(corner1.x, corner1.y, corner2.z);
    m_vertices[5] = Vector3(corner2.x, corner1.y, corner2.z);
    m_vertices[6] = Vector3(corner2.x, corner2.y, corner2.z);
    m_vertices[7] = Vector3(corner1.x, corner2.y, corner2.z);
    
    for(int iVertex=0; iVertex < 8; iVertex++) {
        m_vertices[iVertex] = modelMatrix.dot(m_vertices[iVertex]);
    }
}

KRBoundingVolume::KRBoundingVolume(const KRMat4 &matView, GLfloat fov, GLfloat aspect, GLfloat nearz, GLfloat farz) {
    // Construct a bounding volume representing the volume of the view frustrum
    
    KRMat4 invView = matView;
    invView.invert();
    
    GLfloat r = tan(fov / 2.0);
    
    m_vertices[0] = Vector3(-1.0 * r * nearz * aspect, -1.0 * r * nearz, -nearz);
    m_vertices[1] = Vector3(1.0 * r * nearz * aspect,  -1.0 * r * nearz, -nearz);
    m_vertices[2] = Vector3(1.0 * r * nearz * aspect,   1.0 * r * nearz, -nearz);
    m_vertices[3] = Vector3(-1.0 * r * nearz * aspect,  1.0 * r * nearz, -nearz);
    m_vertices[4] = Vector3(-1.0 * r * farz * aspect, -1.0 * r * farz, -farz);
    m_vertices[5] = Vector3(1.0 * r * farz * aspect,  -1.0 * r * farz, -farz);
    m_vertices[6] = Vector3(1.0 * r * farz * aspect,   1.0 * r * farz, -farz);
    m_vertices[7] = Vector3(-1.0 * r * farz * aspect,  1.0 * r * farz, -farz);
    
    for(int iVertex=0; iVertex < 8; iVertex++) {
        m_vertices[iVertex] = invView.dot(m_vertices[iVertex]);
    }
}

KRBoundingVolume::~KRBoundingVolume() {
    
}

KRBoundingVolume::KRBoundingVolume(const KRBoundingVolume& p) {
    for(int iVertex=0; iVertex < 8; iVertex++) {
        m_vertices[iVertex] = p.m_vertices[iVertex];
    }    
}

KRBoundingVolume& KRBoundingVolume::operator = ( const KRBoundingVolume& p ) {
    for(int iVertex=0; iVertex < 8; iVertex++) {
        m_vertices[iVertex] = p.m_vertices[iVertex];
    }
    return *this;
}

KRBoundingVolume KRBoundingVolume::get_union(const KRBoundingVolume &p) const {
    // Simple, non-aligned bounding box calculated that contains both volumes.
    
    Vector3 minPoint = m_vertices[0], maxPoint = m_vertices[0];
    for(int iVertex=1; iVertex < 8; iVertex++) {
        if(m_vertices[iVertex].x < minPoint.x) {
            minPoint.x = m_vertices[iVertex].x;
        }
        if(m_vertices[iVertex].y < minPoint.y) {
            minPoint.y = m_vertices[iVertex].y;
        }
        if(m_vertices[iVertex].z < minPoint.z) {
            minPoint.z = m_vertices[iVertex].z;
        }
        if(m_vertices[iVertex].x > maxPoint.x) {
            maxPoint.x = m_vertices[iVertex].x;
        }
        if(m_vertices[iVertex].y > maxPoint.y) {
            maxPoint.y = m_vertices[iVertex].y;
        }
        if(m_vertices[iVertex].z > maxPoint.z) {
            maxPoint.z = m_vertices[iVertex].z;
        }
    }
    for(int iVertex=0; iVertex < 8; iVertex++) {
        if(p.m_vertices[iVertex].x < minPoint.x) {
            minPoint.x = p.m_vertices[iVertex].x;
        }
        if(p.m_vertices[iVertex].y < minPoint.y) {
            minPoint.y =p.m_vertices[iVertex].y;
        }
        if(p.m_vertices[iVertex].z < minPoint.z) {
            minPoint.z = p.m_vertices[iVertex].z;
        }
        if(p.m_vertices[iVertex].x > maxPoint.x) {
            maxPoint.x = p.m_vertices[iVertex].x;
        }
        if(p.m_vertices[iVertex].y > maxPoint.y) {
            maxPoint.y = p.m_vertices[iVertex].y;
        }
        if(p.m_vertices[iVertex].z > maxPoint.z) {
            maxPoint.z = p.m_vertices[iVertex].z;
        }
    }
    return KRBoundingVolume(minPoint, maxPoint, KRMat4());
}

bool KRBoundingVolume::test_intersect(const KRBoundingVolume &p) const {
    // Simple, non-aligned bounding box intersection test
    
    Vector3 minPoint = m_vertices[0], maxPoint = m_vertices[0], minPoint2 = p.m_vertices[0], maxPoint2 = p.m_vertices[0];
    for(int iVertex=1; iVertex < 8; iVertex++) {
        if(m_vertices[iVertex].x < minPoint.x) {
            minPoint.x = m_vertices[iVertex].x;
        }
        if(m_vertices[iVertex].y < minPoint.y) {
            minPoint.y = m_vertices[iVertex].y;
        }
        if(m_vertices[iVertex].z < minPoint.z) {
            minPoint.z = m_vertices[iVertex].z;
        }
        if(m_vertices[iVertex].x > maxPoint.x) {
            maxPoint.x = m_vertices[iVertex].x;
        }
        if(m_vertices[iVertex].y > maxPoint.y) {
            maxPoint.y = m_vertices[iVertex].y;
        }
        if(m_vertices[iVertex].z > maxPoint.z) {
            maxPoint.z = m_vertices[iVertex].z;
        }
    }
    for(int iVertex=1; iVertex < 8; iVertex++) {
        if(p.m_vertices[iVertex].x < minPoint2.x) {
            minPoint2.x = p.m_vertices[iVertex].x;
        }
        if(p.m_vertices[iVertex].y < minPoint2.y) {
            minPoint2.y =p.m_vertices[iVertex].y;
        }
        if(p.m_vertices[iVertex].z < minPoint2.z) {
            minPoint2.z = p.m_vertices[iVertex].z;
        }
        if(p.m_vertices[iVertex].x > maxPoint2.x) {
            maxPoint2.x = p.m_vertices[iVertex].x;
        }
        if(p.m_vertices[iVertex].y > maxPoint2.y) {
            maxPoint2.y = p.m_vertices[iVertex].y;
        }
        if(p.m_vertices[iVertex].z > maxPoint2.z) {
            maxPoint2.z = p.m_vertices[iVertex].z;
        }
    }
    
    bool bIntersect = maxPoint.x >= minPoint2.x && maxPoint.y >= minPoint2.y && maxPoint.z >= minPoint2.z
    && minPoint.x <= maxPoint2.x && minPoint.y <= maxPoint2.y && minPoint.z <= maxPoint2.z;
    
    return bIntersect;
}


KRMat4 KRBoundingVolume::calcShadowProj(KRScene *pScene, GLfloat sun_yaw, GLfloat sun_pitch) const {
    KRBoundingVolume sceneVolume = pScene->getExtents();
   
    KRMat4 shadowvp;
    shadowvp.rotate(sun_pitch, X_AXIS);
    shadowvp.rotate(sun_yaw, Y_AXIS);
    shadowvp.invert();
    shadowvp.scale(1.0, 1.0, -1.0);
    
    Vector3 minPointFrustrum = shadowvp.dot(m_vertices[0]), maxPointFrustrum = minPointFrustrum;
    for(int iVertex=1; iVertex < 8; iVertex++) {
        Vector3 v = shadowvp.dot(m_vertices[iVertex]);
        if(v.x < minPointFrustrum.x) {
            minPointFrustrum.x = v.x;
        }
        if(v.y < minPointFrustrum.y) {
            minPointFrustrum.y = v.y;
        }
        if(v.z < minPointFrustrum.z) {
            minPointFrustrum.z = v.z;
        }
        if(v.x > maxPointFrustrum.x) {
            maxPointFrustrum.x = v.x;
        }
        if(v.y > maxPointFrustrum.y) {
            maxPointFrustrum.y = v.y;
        }
        if(v.z > maxPointFrustrum.z) {
            maxPointFrustrum.z = v.z;
        }
    }
    
    
    Vector3 minPointScene = shadowvp.dot(sceneVolume.m_vertices[0]), maxPointScene = minPointScene;
    for(int iVertex=1; iVertex < 8; iVertex++) {
        Vector3 v = shadowvp.dot(sceneVolume.m_vertices[iVertex]);
        if(v.x < minPointScene.x) {
            minPointScene.x = v.x;
        }
        if(v.y < minPointScene.y) {
            minPointScene.y = v.y;
        }
        if(v.z < minPointScene.z) {
            minPointScene.z = v.z;
        }
        if(v.x > maxPointScene.x) {
            maxPointScene.x = v.x;
        }
        if(v.y > maxPointScene.y) {
            maxPointScene.y = v.y;
        }
        if(v.z > maxPointScene.z) {
            maxPointScene.z = v.z;
        }
    }
    
    // Include potential shadow casters outside of view frustrum
    minPointFrustrum.z = minPointScene.z;
    
    if(maxPointScene.z < maxPointFrustrum.z) {
        maxPointFrustrum.z = maxPointScene.z;
    }
    
    /*
    // Include potential shadow casters outside of view frustrum
    GLfloat maxFrustrumDepth = maxPointFrustrum.z;
    
    for(int i=0; i<8; i++) {
        Vector3 v = shadowvp.dot(sceneVolume.m_vertices[i]);
        if(i == 0) {
            minPointFrustrum.z = v.z;
            maxPointFrustrum.z = v.z;
        } else {
            if(v.z < minPointFrustrum.z) {
                minPointFrustrum.z = v.z;
            }
            if(v.z > maxPointFrustrum.z) {
                maxPointFrustrum.z = v.z;
            }
        }
    }
    if(maxPointFrustrum.z > maxFrustrumDepth) {
        maxPointFrustrum.z = maxFrustrumDepth;
    }
     */
    
    shadowvp.translate(-minPointFrustrum.x, -minPointFrustrum.y, -minPointFrustrum.z);
    shadowvp.scale(2.0/(maxPointFrustrum.x - minPointFrustrum.x), 2.0/(maxPointFrustrum.y - minPointFrustrum.y), 1.0/(maxPointFrustrum.z - minPointFrustrum.z));
    shadowvp.translate(-1.0, -1.0, 0.0);
    return shadowvp;
    
}
