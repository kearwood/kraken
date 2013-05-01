//
//  KRAABB.cpp
//  KREngine
//
//  Created by Kearwood Gilbert on 2012-08-30.
//  Copyright (c) 2012 Kearwood Software. All rights reserved.
//

#include "KRAABB.h"
#include "KRMat4.h"
#include "KRVector2.h"
#include "assert.h"

KRAABB::KRAABB()
{
    min = KRVector3::Min();
    max = KRVector3::Max();
}

KRAABB::KRAABB(const KRVector3 &minPoint, const KRVector3 &maxPoint)
{
    min = minPoint;
    max = maxPoint;
}

KRAABB::KRAABB(const KRVector3 &corner1, const KRVector3 &corner2, const KRMat4 &modelMatrix)
{
    for(int iCorner=0; iCorner<8; iCorner++) {
        KRVector3 sourceCornerVertex = KRMat4::DotWDiv(modelMatrix, KRVector3(
                                                 (iCorner & 1) == 0 ? corner1.x : corner2.x,
                                                 (iCorner & 2) == 0 ? corner1.y : corner2.y,
                                                 (iCorner & 4) == 0 ? corner1.z : corner2.z));
        
        
        if(iCorner == 0) {
            min = sourceCornerVertex;
            max = sourceCornerVertex;
        } else {
            if(sourceCornerVertex.x < min.x) min.x = sourceCornerVertex.x;
            if(sourceCornerVertex.y < min.y) min.y = sourceCornerVertex.y;
            if(sourceCornerVertex.z < min.z) min.z = sourceCornerVertex.z;
            if(sourceCornerVertex.x > max.x) max.x = sourceCornerVertex.x;
            if(sourceCornerVertex.y > max.y) max.y = sourceCornerVertex.y;
            if(sourceCornerVertex.z > max.z) max.z = sourceCornerVertex.z;
        }
    }
}

KRAABB::~KRAABB()
{
    
}

KRAABB& KRAABB::operator =(const KRAABB& b)
{
    min = b.min;
    max = b.max;
    
    return *this;
}

bool KRAABB::operator ==(const KRAABB& b) const
{
    return min == b.min && max == b.max;
}

bool KRAABB::operator !=(const KRAABB& b) const
{
    return min != b.min || max != b.max;
}

KRVector3 KRAABB::center() const
{
    return (min + max) * 0.5f;
}

KRVector3 KRAABB::size() const
{
    return max - min;
}

float KRAABB::volume() const
{
    KRVector3 s = size();
    return s.x * s.y * s.z;
}

void KRAABB::scale(const KRVector3 &s)
{
    KRVector3 prev_center = center();
    KRVector3 prev_size = size();
    KRVector3 new_scale = KRVector3(prev_size.x * s.x, prev_size.y * s.y, prev_size.z * s.z) * 0.5f;
    min = prev_center - new_scale;
    max = prev_center + new_scale;
}

void KRAABB::scale(float s)
{
    scale(KRVector3(s));
}

bool KRAABB::operator >(const KRAABB& b) const
{
    // Comparison operators are implemented to allow insertion into sorted containers such as std::set
    if(min > b.min) {
        return true;
    } else if(min < b.min) {
        return false;
    } else if(max > b.max) {
        return true;
    } else {
        return false;
    }

}
bool KRAABB::operator <(const KRAABB& b) const
{
    // Comparison operators are implemented to allow insertion into sorted containers such as std::set
    
    if(min < b.min) {
        return true;
    } else if(min > b.min) {
        return false;
    } else if(max < b.max) {
        return true;
    } else {
        return false;
    }
}

bool KRAABB::intersects(const KRAABB& b) const
{
    // Return true if the two volumes intersect
    return min.x <= b.max.x && min.y <= b.max.y && min.z <= b.max.z && max.x >= b.min.x && max.y >= b.min.y && max.z >= b.max.z;
}

bool KRAABB::contains(const KRAABB &b) const
{
    // Return true if the passed KRAABB is entirely contained within this KRAABB
    return b.min.x >= min.x && b.min.y >= min.y && b.min.z >= min.z && b.max.x <= max.x && b.max.y <= max.y && b.max.z <= max.z;
}

bool KRAABB::contains(const KRVector3 &v) const
{
    return v.x >= min.x && v.x <= max.x && v.y >= min.y && v.y <= max.y && v.z >= min.z && v.z <= max.z;
}

KRAABB KRAABB::Infinite()
{
    return KRAABB(KRVector3::Min(), KRVector3::Max());
}

KRAABB KRAABB::Zero()
{
    return KRAABB(KRVector3::Zero(), KRVector3::Zero());
}

float KRAABB::longest_radius() const
{
    float radius1 = (center() - min).magnitude();
    float radius2 = (max - center()).magnitude();
    return radius1 > radius2 ? radius1 : radius2;
}


bool KRAABB::intersectsLine(const KRVector3 &v1, const KRVector3 &v2) const
{
    KRVector3 dir = KRVector3::Normalize(v2 - v1);
    float length = (v2 - v1).magnitude();
    
    // EZ cases: if the ray starts inside the box, or ends inside
    // the box, then it definitely hits the box.
    // I'm using this code for ray tracing with an octree,
    // so I needed rays that start and end within an
    // octree node to COUNT as hits.
    // You could modify this test to (ray starts inside and ends outside)
    // to qualify as a hit if you wanted to NOT count totally internal rays
    if( contains( v1 ) || contains( v2 ) )
        return true ;
    
    // the algorithm says, find 3 t's,
    KRVector3 t ;
    
    // LARGEST t is the only one we need to test if it's on the face.
    for(int i = 0 ; i < 3 ; i++) {
        if( dir[i] > 0 ) { // CULL BACK FACE
            t[i] = ( min[i] - v1[i] ) / dir[i];
        } else {
            t[i] = ( max[i] - v1[i] ) / dir[i];
        }
    }
    
    int mi = 0;
    if(t[1] > t[mi]) mi = 1;
    if(t[2] > t[mi]) mi = 2;
    if(t[mi] >= 0 && t[mi] <= length) {
        KRVector3 pt = v1 + dir * t[mi];
        
        // check it's in the box in other 2 dimensions
        int o1 = ( mi + 1 ) % 3 ; // i=0: o1=1, o2=2, i=1: o1=2,o2=0 etc.
        int o2 = ( mi + 2 ) % 3 ;
        
        return pt[o1] >= min[o1] && pt[o1] <= max[o1] && pt[o2] >= min[o2] && pt[o2] <= max[o2];
    }
    
    return false ; // the ray did not hit the box.
}

bool KRAABB::intersectsRay(const KRVector3 &v1, const KRVector3 &dir) const
{
    /*
     Fast Ray-Box Intersection
     by Andrew Woo
     from "Graphics Gems", Academic Press, 1990
     */
    
    // FINDME, TODO - Perhaps there is a more efficient algorithm, as we don't actually need the exact coordinate of the intersection
    
    enum {
        RIGHT = 0,
        LEFT = 1,
        MIDDLE = 2
    } quadrant[3];
    
    bool inside = true;
    KRVector3 maxT;
    KRVector3 coord;
	double candidatePlane[3];
    
    // Find candidate planes; this loop can be avoided if rays cast all from the eye(assume perpsective view)
	for (int i=0; i<3; i++)
		if(v1.c[i] < min.c[i]) {
			quadrant[i] = LEFT;
			candidatePlane[i] = min.c[i];
			inside = FALSE;
		}else if (v1.c[i] > max.c[i]) {
			quadrant[i] = RIGHT;
			candidatePlane[i] = max.c[i];
			inside = FALSE;
		}else	{
			quadrant[i] = MIDDLE;
		}
    
	/* Ray v1 inside bounding box */
	if(inside) {
		coord = v1;
		return true;
	}
    
    
	/* Calculate T distances to candidate planes */
	for (int i = 0; i < 3; i++) {
		if (quadrant[i] != MIDDLE && dir[i] !=0.) {
			maxT.c[i] = (candidatePlane[i]-v1.c[i]) / dir[i];
		} else {
			maxT.c[i] = -1.0f;
        }
    }
    
	/* Get largest of the maxT's for final choice of intersection */
	int whichPlane = 0;
	for (int i = 1; i < 3; i++) {
		if (maxT.c[whichPlane] < maxT.c[i]) {
			whichPlane = i;
        }
    }
    
	/* Check final candidate actually inside box */
	if (maxT.c[whichPlane] < 0.0f) return false;
	for (int i = 0; i < 3; i++) {
		if (whichPlane != i) {
			coord[i] = v1.c[i] + maxT.c[whichPlane] *dir[i];
			if (coord[i] < min.c[i] || coord[i] > max.c[i])
				return false;
		} else {
			coord[i] = candidatePlane[i];
		}
    }
	return true;				/* ray hits box */
}

void KRAABB::encapsulate(const KRAABB & b)
{
    if(b.min.x < min.x) min.x = b.min.x;
    if(b.min.y < min.y) min.y = b.min.y;
    if(b.min.z < min.z) min.z = b.min.z;
    
    if(b.max.x > max.x) max.x = b.max.x;
    if(b.max.y > max.y) max.y = b.max.y;
    if(b.max.z > max.z) max.z = b.max.z;
}

KRVector3 KRAABB::nearestPoint(const KRVector3 & v) const
{
    return KRVector3(KRCLAMP(v.x, min.x, max.x), KRCLAMP(v.y, min.y, max.y), KRCLAMP(v.z, min.z, max.z));
}
