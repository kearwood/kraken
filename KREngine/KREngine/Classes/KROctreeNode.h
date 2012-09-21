//
//  KROctreeNode.h
//  KREngine
//
//  Created by Kearwood Gilbert on 2012-08-29.
//  Copyright (c) 2012 Kearwood Software. All rights reserved.
//

#ifndef KROCTREENODE_H
#define KROCTREENODE_H

#import "KREngine-common.h"
#include "KRVector3.h"
#include "KRAABB.h"

class KRNode;

class KROctreeNode {
public:
    KROctreeNode(const KRAABB &bounds);
    KROctreeNode(const KRAABB &bounds, int iChild, KROctreeNode *pChild);
    ~KROctreeNode();
    
    KROctreeNode **getChildren();
    std::set<KRNode *> &getSceneNodes();
    
    void add(KRNode *pNode);
    void remove(KRNode *pNode);
    void update(KRNode *pNode);
    
    KRAABB getBounds();
    
    void setChildNode(int iChild, KROctreeNode *pChild);
    int getChildIndex(KRNode *pNode);
    KRAABB getChildBounds(int iChild);
    bool isEmpty() const;
    
    bool canShrinkRoot() const;
    KROctreeNode *stripChild();
    
    void beginOcclusionQuery();
    void endOcclusionQuery();
    
    
    GLuint m_occlusionQuery;
    bool m_occlusionTested;
    bool m_activeQuery;
private:
    
    KRAABB m_bounds;
    
    KROctreeNode *m_children[8];
    
    std::set<KRNode *>m_sceneNodes;
};


#endif /* defined(KROCTREENODE_H) */
