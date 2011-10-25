//
//  KRMat4.cpp
//  gldemo
//
//  Created by Kearwood Gilbert on 10-09-21.
//  Copyright (c) 2010 Kearwood Software. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "KRMat4.h"

KRMat4::KRMat4() {
    // Default constructor - Initialize with an identity matrix
    static const GLfloat IDENTITY_MATRIX[] = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };
    memcpy(m_mat, IDENTITY_MATRIX, sizeof(GLfloat) * 16);
    
}

KRMat4::KRMat4(GLfloat *pMat) {
    memcpy(m_mat, pMat, sizeof(GLfloat) * 16);
}

KRMat4::~KRMat4() {
    
}

GLfloat *KRMat4::getPointer() {
    return m_mat;
}

// Copy constructor
KRMat4::KRMat4(const KRMat4 &m) {
    
    memcpy(m_mat, m.m_mat, sizeof(GLfloat) * 16);
}

KRMat4& KRMat4::operator=(const KRMat4 &m) {
    if(this != &m) { // Prevent self-assignment.
        memcpy(m_mat, m.m_mat, sizeof(GLfloat) * 16);
    }
    return *this;
}

// Overload compound multiply operator
KRMat4& KRMat4::operator*=(const KRMat4 &m) {
    GLfloat temp[16];
    
    int x,y;
    
    for (x=0; x < 4; x++)
    {
        for(y=0; y < 4; y++)
        {
            temp[y + (x*4)] = (m_mat[x*4] * m.m_mat[y]) +
            (m_mat[(x*4)+1] * m.m_mat[y+4]) +
            (m_mat[(x*4)+2] * m.m_mat[y+8]) +
            (m_mat[(x*4)+3] * m.m_mat[y+12]);
        }
    }
    
    memcpy(m_mat, temp, sizeof(GLfloat) << 4);
    return *this;
}

/*
// Overload multiply operator
KRMat4& KRMat4::operator*(const KRMat4 &m, const KRMat4 &m) {
    KRMat4 result = *this;
    result *= m;
    return result;
}
 */

KRMat4 KRMat4::operator*(const KRMat4 &m) {
    KRMat4 ret = *this;
    ret *= m;
    return ret;
}


/* Generate a perspective view matrix using a field of view angle fov,
 * window aspect ratio, near and far clipping planes */
void KRMat4::perspective(GLfloat fov, GLfloat aspect, GLfloat nearz, GLfloat farz) {
    GLfloat range;
    
    range = tan(fov / 2.0f) * nearz; 
    memset(m_mat, 0, sizeof(GLfloat) * 16);
    m_mat[0] = (2 * nearz) / ((range * aspect) - (-range * aspect));
    m_mat[5] = (2 * nearz) / (2 * range);
    m_mat[10] = -(farz + nearz) / (farz - nearz);
    m_mat[11] = -1;
    m_mat[14] = -(2 * farz * nearz) / (farz - nearz);
    
}

/* Perform translation operations on a matrix */
void KRMat4::translate(GLfloat x, GLfloat y, GLfloat z) {
    KRMat4 newMatrix; // Create new identity matrix
    
    newMatrix.m_mat[12] = x;
    newMatrix.m_mat[13] = y;
    newMatrix.m_mat[14] = z;
    
    *this *= newMatrix;
}

/* Rotate a matrix by an angle on a X, Y, or Z axis */
void KRMat4::rotate(GLfloat angle, AXIS axis) {
    // const GLfloat d2r = 0.0174532925199; /* PI / 180 */
    const int cos1[3] = { 5, 0, 0 };
    const int cos2[3] = { 10, 10, 5 };
    const int sin1[3] = { 6, 2, 1 };
    const int sin2[3] = { 9, 8, 4 };
    
    KRMat4 newMatrix; // Create new identity matrix
    
    newMatrix.m_mat[cos1[axis]] = cos(angle);
    newMatrix.m_mat[sin1[axis]] = -sin(angle);
    newMatrix.m_mat[sin2[axis]] = -newMatrix.m_mat[sin1[axis]];
    newMatrix.m_mat[cos2[axis]] = newMatrix.m_mat[cos1[axis]];
    
    *this *= newMatrix;
}

void KRMat4::scale(GLfloat x, GLfloat y, GLfloat z) {
    KRMat4 newMatrix; // Create new identity matrix
    
    newMatrix.m_mat[0] = x;
    newMatrix.m_mat[5] = y;
    newMatrix.m_mat[10] = z;
    
    *this *= newMatrix;
}

void KRMat4::scale(GLfloat s) {
    scale(s,s,s);
}

void KRMat4::bias() {
    // Initialize with a bias matrix
    static const GLfloat BIAS_MATRIX[] = {
        0.5, 0.0, 0.0, 0.0, 
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
    };
    memcpy(m_mat, BIAS_MATRIX, sizeof(GLfloat) * 16);
}


/* Generate an orthographic view matrix */
void KRMat4::ortho(GLfloat left, GLfloat right, GLfloat top, GLfloat bottom, GLfloat nearz, GLfloat farz) {
    memset(m_mat, 0, sizeof(GLfloat) * 16);
    /*
     m_mat[0] = 2.0f / (right - left);
     m_mat[3] = -(right + left) / (right - left);
     m_mat[5] = 2.0f / (top - bottom);
     m_mat[7] = -(top + bottom) / (top - bottom);
     m_mat[10] = -2.0f / (farz - nearz);
     m_mat[11] = -(farz + nearz) / (farz - nearz);
     m_mat[15] = 1.0f;
     */
    m_mat[0] = 2.0f / (right - left);
    m_mat[5] = 2.0f / (bottom - top);
    m_mat[10] = -1.0f / (farz - nearz);
    m_mat[11] = -nearz / (farz - nearz);
    m_mat[15] = 1.0f;
}

bool KRMat4::invert() {
    /*
    GLfloat inverseTranslation[16];
    GLfloat inverseRotation[16];

	
	
	inverseTranslation[0] = 1 ;	inverseTranslation[4] = 0 ;	inverseTranslation[8] = 0 ;		inverseTranslation[12] = -m_mat[12] ;
	inverseTranslation[1] = 0 ;	inverseTranslation[5] = 1 ;	inverseTranslation[9] = 0 ;		inverseTranslation[13] = -m_mat[13] ;
	inverseTranslation[2] = 0 ;	inverseTranslation[6] = 0 ;	inverseTranslation[10] = 1 ;	inverseTranslation[14] = -m_mat[14] ;
	inverseTranslation[3] = 0 ;	inverseTranslation[7] = 0 ;	inverseTranslation[11] = 0 ;	inverseTranslation[15] = 1 ;
	
	inverseRotation[0] = m_mat[0] ;		inverseRotation[4] = m_mat[1] ;		inverseRotation[8] = m_mat[2] ;		inverseRotation[12] = 0 ;
	inverseRotation[1] = m_mat[4] ;		inverseRotation[5] = m_mat[5] ;		inverseRotation[9] = m_mat[6] ;		inverseRotation[13] = 0 ;
	inverseRotation[2] = m_mat[8] ;		inverseRotation[6] = m_mat[9] ;		inverseRotation[10] = m_mat[10] ;		inverseRotation[14] = 0 ;
	inverseRotation[3] = 0 ;				inverseRotation[7] = 0 ;				inverseRotation[11] = 0 ;				inverseRotation[15] = 1 ;
	
    KRMat4 inverseRotMat(inverseRotation);
    KRMat4 inverseTransMat(inverseTranslation);
    
    KRMat4 m = inverseRotMat * inverseTransMat;
    memcpy(m_mat, m.m_mat, sizeof(GLfloat) * 16);
     */
    
    // Based on gluInvertMatrix implementation
    
    double inv[16], det;
    int i;
    
    inv[0] =   m_mat[5]*m_mat[10]*m_mat[15] - m_mat[5]*m_mat[11]*m_mat[14] - m_mat[9]*m_mat[6]*m_mat[15]
    + m_mat[9]*m_mat[7]*m_mat[14] + m_mat[13]*m_mat[6]*m_mat[11] - m_mat[13]*m_mat[7]*m_mat[10];
    inv[4] =  -m_mat[4]*m_mat[10]*m_mat[15] + m_mat[4]*m_mat[11]*m_mat[14] + m_mat[8]*m_mat[6]*m_mat[15]
    - m_mat[8]*m_mat[7]*m_mat[14] - m_mat[12]*m_mat[6]*m_mat[11] + m_mat[12]*m_mat[7]*m_mat[10];
    inv[8] =   m_mat[4]*m_mat[9]*m_mat[15] - m_mat[4]*m_mat[11]*m_mat[13] - m_mat[8]*m_mat[5]*m_mat[15]
    + m_mat[8]*m_mat[7]*m_mat[13] + m_mat[12]*m_mat[5]*m_mat[11] - m_mat[12]*m_mat[7]*m_mat[9];
    inv[12] = -m_mat[4]*m_mat[9]*m_mat[14] + m_mat[4]*m_mat[10]*m_mat[13] + m_mat[8]*m_mat[5]*m_mat[14]
    - m_mat[8]*m_mat[6]*m_mat[13] - m_mat[12]*m_mat[5]*m_mat[10] + m_mat[12]*m_mat[6]*m_mat[9];
    inv[1] =  -m_mat[1]*m_mat[10]*m_mat[15] + m_mat[1]*m_mat[11]*m_mat[14] + m_mat[9]*m_mat[2]*m_mat[15]
    - m_mat[9]*m_mat[3]*m_mat[14] - m_mat[13]*m_mat[2]*m_mat[11] + m_mat[13]*m_mat[3]*m_mat[10];
    inv[5] =   m_mat[0]*m_mat[10]*m_mat[15] - m_mat[0]*m_mat[11]*m_mat[14] - m_mat[8]*m_mat[2]*m_mat[15]
    + m_mat[8]*m_mat[3]*m_mat[14] + m_mat[12]*m_mat[2]*m_mat[11] - m_mat[12]*m_mat[3]*m_mat[10];
    inv[9] =  -m_mat[0]*m_mat[9]*m_mat[15] + m_mat[0]*m_mat[11]*m_mat[13] + m_mat[8]*m_mat[1]*m_mat[15]
    - m_mat[8]*m_mat[3]*m_mat[13] - m_mat[12]*m_mat[1]*m_mat[11] + m_mat[12]*m_mat[3]*m_mat[9];
    inv[13] =  m_mat[0]*m_mat[9]*m_mat[14] - m_mat[0]*m_mat[10]*m_mat[13] - m_mat[8]*m_mat[1]*m_mat[14]
    + m_mat[8]*m_mat[2]*m_mat[13] + m_mat[12]*m_mat[1]*m_mat[10] - m_mat[12]*m_mat[2]*m_mat[9];
    inv[2] =   m_mat[1]*m_mat[6]*m_mat[15] - m_mat[1]*m_mat[7]*m_mat[14] - m_mat[5]*m_mat[2]*m_mat[15]
    + m_mat[5]*m_mat[3]*m_mat[14] + m_mat[13]*m_mat[2]*m_mat[7] - m_mat[13]*m_mat[3]*m_mat[6];
    inv[6] =  -m_mat[0]*m_mat[6]*m_mat[15] + m_mat[0]*m_mat[7]*m_mat[14] + m_mat[4]*m_mat[2]*m_mat[15]
    - m_mat[4]*m_mat[3]*m_mat[14] - m_mat[12]*m_mat[2]*m_mat[7] + m_mat[12]*m_mat[3]*m_mat[6];
    inv[10] =  m_mat[0]*m_mat[5]*m_mat[15] - m_mat[0]*m_mat[7]*m_mat[13] - m_mat[4]*m_mat[1]*m_mat[15]
    + m_mat[4]*m_mat[3]*m_mat[13] + m_mat[12]*m_mat[1]*m_mat[7] - m_mat[12]*m_mat[3]*m_mat[5];
    inv[14] = -m_mat[0]*m_mat[5]*m_mat[14] + m_mat[0]*m_mat[6]*m_mat[13] + m_mat[4]*m_mat[1]*m_mat[14]
    - m_mat[4]*m_mat[2]*m_mat[13] - m_mat[12]*m_mat[1]*m_mat[6] + m_mat[12]*m_mat[2]*m_mat[5];
    inv[3] =  -m_mat[1]*m_mat[6]*m_mat[11] + m_mat[1]*m_mat[7]*m_mat[10] + m_mat[5]*m_mat[2]*m_mat[11]
    - m_mat[5]*m_mat[3]*m_mat[10] - m_mat[9]*m_mat[2]*m_mat[7] + m_mat[9]*m_mat[3]*m_mat[6];
    inv[7] =   m_mat[0]*m_mat[6]*m_mat[11] - m_mat[0]*m_mat[7]*m_mat[10] - m_mat[4]*m_mat[2]*m_mat[11]
    + m_mat[4]*m_mat[3]*m_mat[10] + m_mat[8]*m_mat[2]*m_mat[7] - m_mat[8]*m_mat[3]*m_mat[6];
    inv[11] = -m_mat[0]*m_mat[5]*m_mat[11] + m_mat[0]*m_mat[7]*m_mat[9] + m_mat[4]*m_mat[1]*m_mat[11]
    - m_mat[4]*m_mat[3]*m_mat[9] - m_mat[8]*m_mat[1]*m_mat[7] + m_mat[8]*m_mat[3]*m_mat[5];
    inv[15] =  m_mat[0]*m_mat[5]*m_mat[10] - m_mat[0]*m_mat[6]*m_mat[9] - m_mat[4]*m_mat[1]*m_mat[10]
    + m_mat[4]*m_mat[2]*m_mat[9] + m_mat[8]*m_mat[1]*m_mat[6] - m_mat[8]*m_mat[2]*m_mat[5];
    
    det = m_mat[0]*inv[0] + m_mat[1]*inv[4] + m_mat[2]*inv[8] + m_mat[3]*inv[12];
    
    if (det == 0) {
        return false;
    }
    
    det = 1.0 / det;
    
    for (i = 0; i < 16; i++) {
        m_mat[i] = inv[i] * det;
    }
    
    
    
    return true;
}


Vector3 KRMat4::dot(const Vector3 &v) const {
    return Vector3(
        v.x * (float)m_mat[0*4 + 0] + v.y * (float)m_mat[1*4 + 0] + v.z * (float)m_mat[2*4 + 0] + (float)m_mat[3*4 + 0],
        v.x * (float)m_mat[0*4 + 1] + v.y * (float)m_mat[1*4 + 1] + v.z * (float)m_mat[2*4 + 1] + (float)m_mat[3*4 + 1],
        v.x * (float)m_mat[0*4 + 2] + v.y * (float)m_mat[1*4 + 2] + v.z * (float)m_mat[2*4 + 2] + (float)m_mat[3*4 + 2]
    );
}
