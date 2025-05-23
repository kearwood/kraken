//
//  KRNode.cpp
//  Kraken Engine
//
//  Copyright 2024 Kearwood Gilbert. All rights reserved.
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

#include "KREngine-common.h"

using namespace hydra;

#include "KRNode.h"
#include "KRLODGroup.h"
#include "KRLODSet.h"
#include "KRPointLight.h"
#include "KRSpotLight.h"
#include "KRDirectionalLight.h"
#include "KRModel.h"
#include "KRCollider.h"
#include "KRParticleSystem.h"
#include "KRParticleSystemNewtonian.h"
#include "KRBone.h"
#include "KRLocator.h"
#include "KRAudioSource.h"
#include "KRAmbientZone.h"
#include "KRReverbZone.h"
#include "KRSprite.h"

/* static */
void KRNode::InitNodeInfo(KrNodeInfo* nodeInfo)
{
  nodeInfo->pName = nullptr;
  nodeInfo->translate = Vector3::Zero();
  nodeInfo->scale = Vector3::One();
  nodeInfo->rotate = Vector3::Zero();
  nodeInfo->pre_rotate = Vector3::Zero();
  nodeInfo->post_rotate = Vector3::Zero();
  nodeInfo->rotate_offset = Vector3::Zero();
  nodeInfo->scale_offset = Vector3::Zero();
  nodeInfo->rotate_pivot = Vector3::Zero();
  nodeInfo->scale_pivot = Vector3::Zero();
}

KrResult KRNode::update(const KrNodeInfo* nodeInfo)
{
  // TODO - Implement name changes

  if (nodeInfo->translate != m_localTranslation ||
      nodeInfo->scale != m_localScale ||
      nodeInfo->rotate != m_localRotation ||
      nodeInfo->pre_rotate != m_initialPreRotation ||
      nodeInfo->post_rotate != m_initialPostRotation ||
      nodeInfo->rotate_offset != m_initialRotationOffset ||
      nodeInfo->scale_offset != m_initialScalingOffset ||
      nodeInfo->rotate_pivot != m_initialRotationPivot ||
      nodeInfo->scale_pivot != m_initialScalingPivot) {

    m_localTranslation = nodeInfo->translate;
    m_initialLocalTranslation = nodeInfo->translate;
    m_localScale = nodeInfo->scale;
    m_initialLocalScale = nodeInfo->scale;
    m_localRotation = nodeInfo->rotate;
    m_initialLocalRotation = nodeInfo->rotate;
    m_initialPreRotation = nodeInfo->pre_rotate;
    m_initialPostRotation = nodeInfo->post_rotate;
    m_initialRotationOffset = nodeInfo->rotate_offset;
    m_initialScalingOffset = nodeInfo->scale_offset;
    m_initialRotationPivot = nodeInfo->rotate_pivot;
    m_initialScalingPivot = nodeInfo->scale_pivot;

    invalidateBindPoseMatrix();
    invalidateModelMatrix();
  }

  return KR_SUCCESS;
}

KRNode::KRNode(KRScene& scene, std::string name) : KRContextObject(scene.getContext())
{
  m_name = name;
  m_localScale = Vector3::One();
  m_localRotation = Vector3::Zero();
  m_localTranslation = Vector3::Zero();
  m_initialLocalTranslation = m_localTranslation;
  m_initialLocalScale = m_localScale;
  m_initialLocalRotation = m_localRotation;



  m_rotationOffset = Vector3::Zero();
  m_scalingOffset = Vector3::Zero();
  m_rotationPivot = Vector3::Zero();
  m_scalingPivot = Vector3::Zero();
  m_preRotation = Vector3::Zero();
  m_postRotation = Vector3::Zero();

  m_initialRotationOffset = Vector3::Zero();
  m_initialScalingOffset = Vector3::Zero();
  m_initialRotationPivot = Vector3::Zero();
  m_initialScalingPivot = Vector3::Zero();
  m_initialPreRotation = Vector3::Zero();
  m_initialPostRotation = Vector3::Zero();

  m_parentNode = nullptr;
  m_previousNode = nullptr;
  m_nextNode = nullptr;
  m_firstChildNode = nullptr;
  m_lastChildNode = nullptr;

  m_pScene = &scene;
  m_modelMatrixValid = false;
  m_inverseModelMatrixValid = false;
  m_bindPoseMatrixValid = false;
  m_activePoseMatrixValid = false;
  m_inverseBindPoseMatrixValid = false;
  m_modelMatrix = Matrix4();
  m_bindPoseMatrix = Matrix4();
  m_activePoseMatrix = Matrix4();
  m_lod_visible = LOD_VISIBILITY_HIDDEN;
  m_scale_compensation = false;
  m_boundsValid = false;

  m_lastRenderFrame = -1000;
  for (int i = 0; i < KRENGINE_NODE_ATTRIBUTE_COUNT; i++) {
    m_animation_mask[i] = false;
  }
}

void KRNode::makeOrphan()
{
  if (m_parentNode == nullptr) {
    // Already an orphan
    return;
  }
  if (m_nextNode != nullptr) {
    m_nextNode->m_previousNode = m_previousNode;
  }
  if (m_previousNode != nullptr) {
    m_previousNode->m_nextNode = m_nextNode;
  }
  if (m_previousNode == nullptr) {
    m_parentNode->m_firstChildNode = m_nextNode;
  }
  if (m_nextNode == nullptr) {
    m_parentNode->m_lastChildNode = m_previousNode;
  }
  m_parentNode->childRemoved(this);
  m_parentNode = nullptr;
  m_nextNode = nullptr;
  m_previousNode = nullptr;
}

KRNode::~KRNode()
{
  while (m_firstChildNode != nullptr) {
    delete m_firstChildNode;
  }

  makeOrphan();

  for (std::set<KRBehavior*>::iterator itr = m_behaviors.begin(); itr != m_behaviors.end(); itr++) {
    delete* itr;
  }
  m_behaviors.clear();

  getScene().notify_sceneGraphDelete(this);
}

void KRNode::setScaleCompensation(bool scale_compensation)
{
  if (m_scale_compensation != scale_compensation) {
    m_scale_compensation = scale_compensation;
    invalidateModelMatrix();
    invalidateBindPoseMatrix();
  }
}
bool KRNode::getScaleCompensation()
{
  return m_scale_compensation;
}

void KRNode::childRemoved(KRNode* child_node)
{
  invalidateBounds();
  getScene().notify_sceneGraphModify(this);
}

bool KRNode::isFirstSibling() const
{
  return m_previousNode == nullptr;
}
bool KRNode::isLastSibling() const
{
  return m_nextNode == nullptr;
}

void KRNode::appendChild(KRNode* child)
{
  child->makeOrphan();
  child->m_parentNode = this;
  if (m_firstChildNode == nullptr) {
    m_firstChildNode = child;
    m_lastChildNode = child;
  } else {
    m_lastChildNode->m_nextNode = child;
    child->m_previousNode = m_lastChildNode;
    m_lastChildNode = child;
  }
  child->setLODVisibility(m_lod_visible); // Child node inherits LOD visibility status from parent
}

void KRNode::prependChild(KRNode* child)
{
  child->makeOrphan();
  child->m_parentNode = this;
  if (m_firstChildNode == nullptr) {
    m_firstChildNode = child;
    m_lastChildNode = child;
  } else {
    m_firstChildNode->m_previousNode = child;
    child->m_nextNode = m_firstChildNode;
    m_firstChildNode = child;
  }
  child->setLODVisibility(m_lod_visible); // Child node inherits LOD visibility status from parent
}
void KRNode::insertBefore(KRNode* child)
{
  assert(m_parentNode != NULL); // There can only be one root node
  child->makeOrphan();
  child->m_parentNode = m_parentNode;
  child->m_nextNode = this;
  child->m_previousNode = m_previousNode;
  m_previousNode = child;

  child->setLODVisibility(m_lod_visible); // Child node inherits LOD visibility status from parent
}
void KRNode::insertAfter(KRNode* child)
{
  assert(m_parentNode != NULL); // There can only be one root node
  child->makeOrphan();

  child->m_parentNode = m_parentNode;
  child->m_previousNode = this;
  child->m_nextNode = m_nextNode;
  m_nextNode = child;

  child->setLODVisibility(m_lod_visible); // Child node inherits LOD visibility status from parent
}

tinyxml2::XMLElement* KRNode::saveXML(tinyxml2::XMLNode* parent)
{
  tinyxml2::XMLDocument* doc = parent->GetDocument();
  tinyxml2::XMLElement* e = doc->NewElement(getElementName().c_str());
  tinyxml2::XMLNode* n = parent->InsertEndChild(e);
  e->SetAttribute("name", m_name.c_str());
  kraken::setXMLAttribute("translate", e, m_localTranslation, Vector3::Zero());
  kraken::setXMLAttribute("scale", e, m_localScale, Vector3::One());
  kraken::setXMLAttribute("rotate", e, (m_localRotation * (180.0f / (float)M_PI)), Vector3::Zero());
  kraken::setXMLAttribute("rotate_offset", e, m_rotationOffset, Vector3::Zero());
  kraken::setXMLAttribute("scale_offset", e, m_scalingOffset, Vector3::Zero());
  kraken::setXMLAttribute("rotate_pivot", e, m_rotationPivot, Vector3::Zero());
  kraken::setXMLAttribute("scale_pivot", e, m_scalingPivot, Vector3::Zero());
  kraken::setXMLAttribute("pre_rotate", e, (m_preRotation * (180.0f / (float)M_PI)), Vector3::Zero());
  kraken::setXMLAttribute("post_rotate", e, (m_postRotation * (180.0f / (float)M_PI)), Vector3::Zero());

  for (KRNode* child = m_firstChildNode; child != nullptr; child = child->m_nextNode) {
    child->saveXML(n);
  }
  return e;
}

KrResult KRNode::createNode(const KrCreateNodeInfo* pCreateNodeInfo, KRScene* scene, KRNode** node)
{
  switch (pCreateNodeInfo->node.sType) {
  case KR_STRUCTURE_TYPE_NODE_CAMERA:
    *node = new KRCamera(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_LOD_SET:
    *node = new KRLODSet(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_LOD_GROUP:
    *node = new KRLODGroup(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_POINT_LIGHT:
    *node = new KRPointLight(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_DIRECTIONAL_LIGHT:
    *node = new KRDirectionalLight(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_SPOT_LIGHT:
    *node = new KRSpotLight(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_SPRITE:
    *node = new KRSprite(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_MODEL:
    *node = new KRModel(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_COLLIDER:
    *node = new KRCollider(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_BONE:
    *node = new KRBone(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_LOCATOR:
    *node = new KRLocator(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_AUDIO_SOURCE:
    *node = new KRAudioSource(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_AMBIENT_ZONE:
    *node = new KRAmbientZone(*scene, pCreateNodeInfo->node.pName);
    break;
  case KR_STRUCTURE_TYPE_NODE_REVERB_ZONE:
    *node = new KRReverbZone(*scene, pCreateNodeInfo->node.pName);
    break;
  default:
    return KR_ERROR_NOT_IMPLEMENTED;
  }
  KrResult res = (*node)->update(&pCreateNodeInfo->node);
  if (res != KR_SUCCESS) {
    delete *node;
    *node = nullptr;
    return res;
  }
  return KR_SUCCESS;
}

void KRNode::loadXML(tinyxml2::XMLElement* e)
{
  m_name = e->Attribute("name");
  m_localTranslation = kraken::getXMLAttribute("translate", e, Vector3::Zero());
  m_localScale = kraken::getXMLAttribute("scale", e, Vector3::One());
  m_localRotation = kraken::getXMLAttribute("rotate", e, Vector3::Zero());
  m_localRotation *= (float)M_PI / 180.0f; // Convert degrees to radians
  m_preRotation = kraken::getXMLAttribute("pre_rotate", e, Vector3::Zero());
  m_preRotation *= (float)M_PI / 180.0f; // Convert degrees to radians
  m_postRotation = kraken::getXMLAttribute("post_rotate", e, Vector3::Zero());
  m_postRotation *= (float)M_PI / 180.0f; // Convert degrees to radians

  m_rotationOffset = kraken::getXMLAttribute("rotate_offset", e, Vector3::Zero());
  m_scalingOffset = kraken::getXMLAttribute("scale_offset", e, Vector3::Zero());
  m_rotationPivot = kraken::getXMLAttribute("rotate_pivot", e, Vector3::Zero());
  m_scalingPivot = kraken::getXMLAttribute("scale_pivot", e, Vector3::Zero());

  m_initialLocalTranslation = m_localTranslation;
  m_initialLocalScale = m_localScale;
  m_initialLocalRotation = m_localRotation;

  m_initialRotationOffset = m_rotationOffset;
  m_initialScalingOffset = m_scalingOffset;
  m_initialRotationPivot = m_rotationPivot;
  m_initialScalingPivot = m_scalingPivot;
  m_initialPreRotation = m_preRotation;
  m_initialPostRotation = m_postRotation;

  m_bindPoseMatrixValid = false;
  m_activePoseMatrixValid = false;
  m_inverseBindPoseMatrixValid = false;
  m_modelMatrixValid = false;
  m_inverseModelMatrixValid = false;

  for (tinyxml2::XMLElement* child_element = e->FirstChildElement(); child_element != NULL; child_element = child_element->NextSiblingElement()) {
    const char* szElementName = child_element->Name();
    if (strcmp(szElementName, "behavior") == 0) {
      KRBehavior* behavior = KRBehavior::LoadXML(this, child_element);
      if (behavior) {
        addBehavior(behavior);
        behavior->init();
      }
    } else {
      KRNode* child_node = KRNode::LoadXML(getScene(), child_element);

      if (child_node) {
        appendChild(child_node);
      }
    }
  }
}

void KRNode::setLocalTranslation(const Vector3& v, bool set_original)
{
  m_localTranslation = v;
  if (set_original) {
    m_initialLocalTranslation = v;
    invalidateBindPoseMatrix();
  }
  invalidateModelMatrix();
}

void KRNode::setWorldTranslation(const Vector3& v)
{
  if (m_parentNode) {
    setLocalTranslation(Matrix4::Dot(m_parentNode->getInverseModelMatrix(), v));
  } else {
    setLocalTranslation(v);
  }
}


void KRNode::setWorldRotation(const Vector3& v)
{
  if (m_parentNode) {
    setLocalRotation((Quaternion::Create(v) * -m_parentNode->getWorldRotation()).eulerXYZ());
    setPreRotation(Vector3::Zero());
    setPostRotation(Vector3::Zero());
  } else {
    setLocalRotation(v);
    setPreRotation(Vector3::Zero());
    setPostRotation(Vector3::Zero());
  }
}


void KRNode::setWorldScale(const Vector3& v)
{
  if (m_parentNode) {
    setLocalScale(Matrix4::DotNoTranslate(m_parentNode->getInverseModelMatrix(), v));
  } else {
    setLocalScale(v);
  }
}

void KRNode::setLocalScale(const Vector3& v, bool set_original)
{
  m_localScale = v;
  if (set_original) {
    m_initialLocalScale = v;
    invalidateBindPoseMatrix();
  }
  invalidateModelMatrix();
}

void KRNode::setLocalRotation(const Vector3& v, bool set_original)
{
  m_localRotation = v;
  if (set_original) {
    m_initialLocalRotation = v;
    invalidateBindPoseMatrix();
  }
  invalidateModelMatrix();
}


void KRNode::setRotationOffset(const Vector3& v, bool set_original)
{
  m_rotationOffset = v;
  if (set_original) {
    m_initialRotationOffset = v;
    invalidateBindPoseMatrix();
  }
  invalidateModelMatrix();
}

void KRNode::setScalingOffset(const Vector3& v, bool set_original)
{
  m_scalingOffset = v;
  if (set_original) {
    m_initialScalingOffset = v;
    invalidateBindPoseMatrix();
  }
  invalidateModelMatrix();
}

void KRNode::setRotationPivot(const Vector3& v, bool set_original)
{
  m_rotationPivot = v;
  if (set_original) {
    m_initialRotationPivot = v;
    invalidateBindPoseMatrix();
  }
  invalidateModelMatrix();
}
void KRNode::setScalingPivot(const Vector3& v, bool set_original)
{
  m_scalingPivot = v;
  if (set_original) {
    m_initialScalingPivot = v;
    invalidateBindPoseMatrix();
  }
  invalidateModelMatrix();
}
void KRNode::setPreRotation(const Vector3& v, bool set_original)
{
  m_preRotation = v;
  if (set_original) {
    m_initialPreRotation = v;
    invalidateBindPoseMatrix();
  }
  invalidateModelMatrix();
}
void KRNode::setPostRotation(const Vector3& v, bool set_original)
{
  m_postRotation = v;
  if (set_original) {
    m_initialPostRotation = v;
    invalidateBindPoseMatrix();
  }
  invalidateModelMatrix();
}

const Vector3& KRNode::getRotationOffset()
{
  return m_rotationOffset;
}
const Vector3& KRNode::getScalingOffset()
{
  return m_scalingOffset;
}
const Vector3& KRNode::getRotationPivot()
{
  return m_rotationPivot;
}
const Vector3& KRNode::getScalingPivot()
{
  return m_scalingPivot;
}
const Vector3& KRNode::getPreRotation()
{
  return m_preRotation;
}
const Vector3& KRNode::getPostRotation()
{
  return m_postRotation;
}
const Vector3& KRNode::getInitialRotationOffset()
{
  return m_initialRotationOffset;
}
const Vector3& KRNode::getInitialScalingOffset()
{
  return m_initialScalingOffset;
}
const Vector3& KRNode::getInitialRotationPivot()
{
  return m_initialRotationPivot;
}
const Vector3& KRNode::getInitialScalingPivot()
{
  return m_initialScalingPivot;
}
const Vector3& KRNode::getInitialPreRotation()
{
  return m_initialPreRotation;
}
const Vector3& KRNode::getInitialPostRotation()
{
  return m_initialPostRotation;
}

const Vector3& KRNode::getLocalTranslation()
{
  return m_localTranslation;
}
const Vector3& KRNode::getLocalScale()
{
  return m_localScale;
}
const Vector3& KRNode::getLocalRotation()
{
  return m_localRotation;
}

const Vector3& KRNode::getInitialLocalTranslation()
{
  return m_initialLocalTranslation;
}
const Vector3& KRNode::getInitialLocalScale()
{
  return m_initialLocalScale;
}
const Vector3& KRNode::getInitialLocalRotation()
{
  return m_initialLocalRotation;
}

const Vector3 KRNode::getWorldTranslation()
{
  return localToWorld(Vector3::Zero());
}

const Vector3 KRNode::getWorldScale()
{
  return Matrix4::DotNoTranslate(getModelMatrix(), m_localScale);
}

std::string KRNode::getElementName()
{
  return "node";
}

KRNode* KRNode::LoadXML(KRScene& scene, tinyxml2::XMLElement* e)
{
  KRNode* new_node = NULL;
  const char* szElementName = e->Name();
  const char* szName = e->Attribute("name");
  if (strcmp(szElementName, "node") == 0) {
    new_node = new KRNode(scene, szName);
  } else if (strcmp(szElementName, "lod_set") == 0) {
    new_node = new KRLODSet(scene, szName);
  } else if (strcmp(szElementName, "lod_group") == 0) {
    new_node = new KRLODGroup(scene, szName);
  } else if (strcmp(szElementName, "point_light") == 0) {
    new_node = new KRPointLight(scene, szName);
  } else if (strcmp(szElementName, "directional_light") == 0) {
    new_node = new KRDirectionalLight(scene, szName);
  } else if (strcmp(szElementName, "spot_light") == 0) {
    new_node = new KRSpotLight(scene, szName);
  } else if (strcmp(szElementName, "particles_newtonian") == 0) {
    new_node = new KRParticleSystemNewtonian(scene, szName);
  } else if (strcmp(szElementName, "sprite") == 0) {
    new_node = new KRSprite(scene, szName);
  } else if (strcmp(szElementName, "model") == 0) {
    float lod_min_coverage = 0.0f;
    if (e->QueryFloatAttribute("lod_min_coverage", &lod_min_coverage) != tinyxml2::XML_SUCCESS) {
      lod_min_coverage = 0.0f;
    }
    bool receives_shadow = true;
    if (e->QueryBoolAttribute("receives_shadow", &receives_shadow) != tinyxml2::XML_SUCCESS) {
      receives_shadow = true;
    }
    bool faces_camera = false;
    if (e->QueryBoolAttribute("faces_camera", &faces_camera) != tinyxml2::XML_SUCCESS) {
      faces_camera = false;
    }
    float rim_power = 0.0f;
    if (e->QueryFloatAttribute("rim_power", &rim_power) != tinyxml2::XML_SUCCESS) {
      rim_power = 0.0f;
    }
    Vector3 rim_color = Vector3::Zero();
    rim_color = kraken::getXMLAttribute("rim_color", e, Vector3::Zero());
    new_node = new KRModel(scene, szName, e->Attribute("mesh"), e->Attribute("light_map"), lod_min_coverage, receives_shadow, faces_camera, rim_color, rim_power);
  } else if (strcmp(szElementName, "collider") == 0) {
    new_node = new KRCollider(scene, szName, e->Attribute("mesh"), 65535, 1.0f);
  } else if (strcmp(szElementName, "bone") == 0) {
    new_node = new KRBone(scene, szName);
  } else if (strcmp(szElementName, "locator") == 0) {
    new_node = new KRLocator(scene, szName);
  } else if (strcmp(szElementName, "audio_source") == 0) {
    new_node = new KRAudioSource(scene, szName);
  } else if (strcmp(szElementName, "ambient_zone") == 0) {
    new_node = new KRAmbientZone(scene, szName);
  } else if (strcmp(szElementName, "reverb_zone") == 0) {
    new_node = new KRReverbZone(scene, szName);
  } else if (strcmp(szElementName, "camera") == 0) {
    new_node = new KRCamera(scene, szName);
  }

  if (new_node) {
    new_node->loadXML(e);
  }

  return new_node;
}

void KRNode::render(RenderInfo& ri)
{
  if (m_lod_visible <= LOD_VISIBILITY_PRESTREAM) return;

  m_lastRenderFrame = getContext().getCurrentFrame();
}

KRNode* KRNode::getParent()
{
  return m_parentNode;
}

const std::string& KRNode::getName() const
{
  return m_name;
}

KRScene& KRNode::getScene()
{
  return *m_pScene;
}

AABB KRNode::getBounds()
{
  if (!m_boundsValid) {
    AABB bounds = AABB::Zero();

    for (KRNode* child = m_firstChildNode; child != nullptr; child = child->m_nextNode) {
      if (child->getBounds() != AABB::Zero()) {
        if (child->isFirstSibling()) {
          bounds = child->getBounds();
        } else {
          bounds.encapsulate(child->getBounds());
        }
      }
    }

    m_bounds = bounds;
    m_boundsValid = true;
  }
  return m_bounds;
}

void KRNode::invalidateModelMatrix()
{
  m_modelMatrixValid = false;
  m_activePoseMatrixValid = false;
  m_inverseModelMatrixValid = false;
  for (KRNode* child = m_firstChildNode; child != nullptr; child = child->m_nextNode) {
    child->invalidateModelMatrix();
  }

  invalidateBounds();
  getScene().notify_sceneGraphModify(this);
}

void KRNode::invalidateBindPoseMatrix()
{
  m_bindPoseMatrixValid = false;
  m_inverseBindPoseMatrixValid = false;
  for (KRNode* child = m_firstChildNode; child != nullptr; child = child->m_nextNode) {
    child->invalidateBindPoseMatrix();
  }
}

const Matrix4& KRNode::getModelMatrix()
{

  if (!m_modelMatrixValid) {
    m_modelMatrix = Matrix4();

    bool parent_is_bone = false;
    if (dynamic_cast<KRBone*>(m_parentNode)) {
      parent_is_bone = true;
    }

    if (getScaleCompensation() && parent_is_bone) {


      // WorldTransform = ParentWorldTransform * T * Roff * Rp * Rpre * R * Rpost * Rp-1 * Soff * Sp * S * Sp-1
      m_modelMatrix = Matrix4::Translation(-m_scalingPivot)
        * Matrix4::Scaling(m_localScale)
        * Matrix4::Translation(m_scalingPivot)
        * Matrix4::Translation(m_scalingOffset)
        * Matrix4::Translation(-m_rotationPivot)
        //* (Quaternion(m_postRotation) * Quaternion(m_localRotation) * Quaternion(m_preRotation)).rotationMatrix()
        * Matrix4::Rotation(m_postRotation)
        * Matrix4::Rotation(m_localRotation)
        * Matrix4::Rotation(m_preRotation)
        * Matrix4::Translation(m_rotationPivot)
        * Matrix4::Translation(m_rotationOffset);

      if (m_parentNode) {

        m_modelMatrix.rotate(m_parentNode->getWorldRotation());
        m_modelMatrix.translate(Matrix4::Dot(m_parentNode->getModelMatrix(), m_localTranslation));
      } else {
        m_modelMatrix.translate(m_localTranslation);
      }
    } else {

      // WorldTransform = ParentWorldTransform * T * Roff * Rp * Rpre * R * Rpost * Rp-1 * Soff * Sp * S * Sp-1
      m_modelMatrix = Matrix4::Translation(-m_scalingPivot)
        * Matrix4::Scaling(m_localScale)
        * Matrix4::Translation(m_scalingPivot)
        * Matrix4::Translation(m_scalingOffset)
        * Matrix4::Translation(-m_rotationPivot)
        //* (Quaternion(m_postRotation) * Quaternion(m_localRotation) * Quaternion(m_preRotation)).rotationMatrix()
        * Matrix4::Rotation(m_postRotation)
        * Matrix4::Rotation(m_localRotation)
        * Matrix4::Rotation(m_preRotation)
        * Matrix4::Translation(m_rotationPivot)
        * Matrix4::Translation(m_rotationOffset)
        * Matrix4::Translation(m_localTranslation);

      if (m_parentNode) {
        m_modelMatrix *= m_parentNode->getModelMatrix();
      }
    }

    m_modelMatrixValid = true;

  }
  return m_modelMatrix;
}

const Matrix4& KRNode::getBindPoseMatrix()
{
  if (!m_bindPoseMatrixValid) {
    m_bindPoseMatrix = Matrix4();

    bool parent_is_bone = false;
    if (dynamic_cast<KRBone*>(m_parentNode)) {
      parent_is_bone = true;
    }

    if (getScaleCompensation() && parent_is_bone) {
      m_bindPoseMatrix = Matrix4::Translation(-m_initialScalingPivot)
        * Matrix4::Scaling(m_initialLocalScale)
        * Matrix4::Translation(m_initialScalingPivot)
        * Matrix4::Translation(m_initialScalingOffset)
        * Matrix4::Translation(-m_initialRotationPivot)
        //* (Quaternion(m_initialPostRotation) * Quaternion(m_initialLocalRotation) * Quaternion(m_initialPreRotation)).rotationMatrix()
        * Matrix4::Rotation(m_initialPostRotation)
        * Matrix4::Rotation(m_initialLocalRotation)
        * Matrix4::Rotation(m_initialPreRotation)
        * Matrix4::Translation(m_initialRotationPivot)
        * Matrix4::Translation(m_initialRotationOffset);
      //m_bindPoseMatrix.translate(m_localTranslation);
      if (m_parentNode) {

        m_bindPoseMatrix.rotate(m_parentNode->getBindPoseWorldRotation());
        m_bindPoseMatrix.translate(Matrix4::Dot(m_parentNode->getBindPoseMatrix(), m_localTranslation));
      } else {
        m_bindPoseMatrix.translate(m_localTranslation);
      }
    } else {

      // WorldTransform = ParentWorldTransform * T * Roff * Rp * Rpre * R * Rpost * Rp-1 * Soff * Sp * S * Sp-1

      m_bindPoseMatrix = Matrix4::Translation(-m_initialScalingPivot)
        * Matrix4::Scaling(m_initialLocalScale)
        * Matrix4::Translation(m_initialScalingPivot)
        * Matrix4::Translation(m_initialScalingOffset)
        * Matrix4::Translation(-m_initialRotationPivot)
        // * (Quaternion(m_initialPostRotation) * Quaternion(m_initialLocalRotation) * Quaternion(m_initialPreRotation)).rotationMatrix()
        * Matrix4::Rotation(m_initialPostRotation)
        * Matrix4::Rotation(m_initialLocalRotation)
        * Matrix4::Rotation(m_initialPreRotation)
        * Matrix4::Translation(m_initialRotationPivot)
        * Matrix4::Translation(m_initialRotationOffset)
        * Matrix4::Translation(m_initialLocalTranslation);

      if (m_parentNode && parent_is_bone) {

        m_bindPoseMatrix *= m_parentNode->getBindPoseMatrix();
      }
    }

    m_bindPoseMatrixValid = true;

  }
  return m_bindPoseMatrix;
}

const Matrix4& KRNode::getActivePoseMatrix()
{

  if (!m_activePoseMatrixValid) {
    m_activePoseMatrix = Matrix4();

    bool parent_is_bone = false;
    if (dynamic_cast<KRBone*>(m_parentNode)) {
      parent_is_bone = true;
    }

    if (getScaleCompensation() && parent_is_bone) {
      m_activePoseMatrix = Matrix4::Translation(-m_scalingPivot)
        * Matrix4::Scaling(m_localScale)
        * Matrix4::Translation(m_scalingPivot)
        * Matrix4::Translation(m_scalingOffset)
        * Matrix4::Translation(-m_rotationPivot)
        * Matrix4::Rotation(m_postRotation)
        * Matrix4::Rotation(m_localRotation)
        * Matrix4::Rotation(m_preRotation)
        * Matrix4::Translation(m_rotationPivot)
        * Matrix4::Translation(m_rotationOffset);

      if (m_parentNode) {

        m_activePoseMatrix.rotate(m_parentNode->getActivePoseWorldRotation());
        m_activePoseMatrix.translate(Matrix4::Dot(m_parentNode->getActivePoseMatrix(), m_localTranslation));
      } else {
        m_activePoseMatrix.translate(m_localTranslation);
      }
    } else {

      // WorldTransform = ParentWorldTransform * T * Roff * Rp * Rpre * R * Rpost * Rp-1 * Soff * Sp * S * Sp-1
      m_activePoseMatrix = Matrix4::Translation(-m_scalingPivot)
        * Matrix4::Scaling(m_localScale)
        * Matrix4::Translation(m_scalingPivot)
        * Matrix4::Translation(m_scalingOffset)
        * Matrix4::Translation(-m_rotationPivot)
        * Matrix4::Rotation(m_postRotation)
        * Matrix4::Rotation(m_localRotation)
        * Matrix4::Rotation(m_preRotation)
        * Matrix4::Translation(m_rotationPivot)
        * Matrix4::Translation(m_rotationOffset)
        * Matrix4::Translation(m_localTranslation);


      if (m_parentNode && parent_is_bone) {
        m_activePoseMatrix *= m_parentNode->getActivePoseMatrix();
      }
    }

    m_activePoseMatrixValid = true;

  }
  return m_activePoseMatrix;

}

const Quaternion KRNode::getWorldRotation()
{
  Quaternion world_rotation = Quaternion::Create(m_postRotation) * Quaternion::Create(m_localRotation) * Quaternion::Create(m_preRotation);
  if (m_parentNode) {
    world_rotation = world_rotation * m_parentNode->getWorldRotation();
  }
  return world_rotation;
}

const Quaternion KRNode::getBindPoseWorldRotation()
{
  Quaternion world_rotation = Quaternion::Create(m_initialPostRotation) * Quaternion::Create(m_initialLocalRotation) * Quaternion::Create(m_initialPreRotation);
  if (dynamic_cast<KRBone*>(m_parentNode)) {
    world_rotation = world_rotation * m_parentNode->getBindPoseWorldRotation();
  }
  return world_rotation;
}

const Quaternion KRNode::getActivePoseWorldRotation()
{
  Quaternion world_rotation = Quaternion::Create(m_postRotation) * Quaternion::Create(m_localRotation) * Quaternion::Create(m_preRotation);
  if (dynamic_cast<KRBone*>(m_parentNode)) {
    world_rotation = world_rotation * m_parentNode->getActivePoseWorldRotation();
  }
  return world_rotation;
}

const Matrix4& KRNode::getInverseModelMatrix()
{
  if (!m_inverseModelMatrixValid) {
    m_inverseModelMatrix = Matrix4::Invert(getModelMatrix());
  }
  return m_inverseModelMatrix;
}

const Matrix4& KRNode::getInverseBindPoseMatrix()
{
  if (!m_inverseBindPoseMatrixValid) {
    m_inverseBindPoseMatrix = Matrix4::Invert(getBindPoseMatrix());
    m_inverseBindPoseMatrixValid = true;
  }
  return m_inverseBindPoseMatrix;
}

void KRNode::physicsUpdate(float deltaTime)
{
  const long MIN_DISPLAY_FRAMES = 10;
  bool visible = m_lastRenderFrame + MIN_DISPLAY_FRAMES >= getContext().getCurrentFrame();
  for (std::set<KRBehavior*>::iterator itr = m_behaviors.begin(); itr != m_behaviors.end(); itr++) {
    (*itr)->update(deltaTime);
    if (visible) {
      (*itr)->visibleUpdate(deltaTime);
    }
  }
}

bool KRNode::hasPhysics()
{
  return m_behaviors.size() > 0;
}

void KRNode::SetAttribute(node_attribute_type attrib, float v)
{
  if (m_animation_mask[attrib]) return;

  const float DEGREES_TO_RAD = (float)M_PI / 180.0f;

  //printf("%s - ", m_name.c_str());
  switch (attrib) {
  case KRENGINE_NODE_ATTRIBUTE_TRANSLATE_X:
    setLocalTranslation(Vector3::Create(v, m_localTranslation.y, m_localTranslation.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_TRANSLATE_Y:
    setLocalTranslation(Vector3::Create(m_localTranslation.x, v, m_localTranslation.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_TRANSLATE_Z:
    setLocalTranslation(Vector3::Create(m_localTranslation.x, m_localTranslation.y, v));
    break;
  case KRENGINE_NODE_ATTRIBUTE_SCALE_X:
    setLocalScale(Vector3::Create(v, m_localScale.y, m_localScale.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_SCALE_Y:
    setLocalScale(Vector3::Create(m_localScale.x, v, m_localScale.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_SCALE_Z:
    setLocalScale(Vector3::Create(m_localScale.x, m_localScale.y, v));
    break;
  case KRENGINE_NODE_ATTRIBUTE_ROTATE_X:
    setLocalRotation(Vector3::Create(v * DEGREES_TO_RAD, m_localRotation.y, m_localRotation.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_ROTATE_Y:
    setLocalRotation(Vector3::Create(m_localRotation.x, v * DEGREES_TO_RAD, m_localRotation.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_ROTATE_Z:
    setLocalRotation(Vector3::Create(m_localRotation.x, m_localRotation.y, v * DEGREES_TO_RAD));
    break;


  case KRENGINE_NODE_ATTRIBUTE_PRE_ROTATION_X:
    setPreRotation(Vector3::Create(v * DEGREES_TO_RAD, m_preRotation.y, m_preRotation.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_PRE_ROTATION_Y:
    setPreRotation(Vector3::Create(m_preRotation.x, v * DEGREES_TO_RAD, m_preRotation.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_PRE_ROTATION_Z:
    setPreRotation(Vector3::Create(m_preRotation.x, m_preRotation.y, v * DEGREES_TO_RAD));
    break;
  case KRENGINE_NODE_ATTRIBUTE_POST_ROTATION_X:
    setPostRotation(Vector3::Create(v * DEGREES_TO_RAD, m_postRotation.y, m_postRotation.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_POST_ROTATION_Y:
    setPostRotation(Vector3::Create(m_postRotation.x, v * DEGREES_TO_RAD, m_postRotation.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_POST_ROTATION_Z:
    setPostRotation(Vector3::Create(m_postRotation.x, m_postRotation.y, v * DEGREES_TO_RAD));
    break;
  case KRENGINE_NODE_ATTRIBUTE_ROTATION_PIVOT_X:
    setRotationPivot(Vector3::Create(v, m_rotationPivot.y, m_rotationPivot.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_ROTATION_PIVOT_Y:
    setRotationPivot(Vector3::Create(m_rotationPivot.x, v, m_rotationPivot.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_ROTATION_PIVOT_Z:
    setRotationPivot(Vector3::Create(m_rotationPivot.x, m_rotationPivot.y, v));
    break;
  case KRENGINE_NODE_ATTRIBUTE_SCALE_PIVOT_X:
    setScalingPivot(Vector3::Create(v, m_scalingPivot.y, m_scalingPivot.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_SCALE_PIVOT_Y:
    setScalingPivot(Vector3::Create(m_scalingPivot.x, v, m_scalingPivot.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_SCALE_PIVOT_Z:
    setScalingPivot(Vector3::Create(m_scalingPivot.x, m_scalingPivot.y, v));
    break;
  case KRENGINE_NODE_ATTRIBUTE_ROTATE_OFFSET_X:
    setRotationOffset(Vector3::Create(v, m_rotationOffset.y, m_rotationOffset.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_ROTATE_OFFSET_Y:
    setRotationOffset(Vector3::Create(m_rotationOffset.x, v, m_rotationOffset.z));
    break;
  case KRENGINE_NODE_ATTRIBUTE_ROTATE_OFFSET_Z:
    setRotationOffset(Vector3::Create(m_rotationOffset.x, m_rotationOffset.y, v));
    break;
  case KRENGINE_NODE_SCALE_OFFSET_X:
    setScalingOffset(Vector3::Create(v, m_scalingOffset.y, m_scalingOffset.z));
    break;
  case KRENGINE_NODE_SCALE_OFFSET_Y:
    setScalingOffset(Vector3::Create(m_scalingOffset.x, v, m_scalingOffset.z));
    break;
  case KRENGINE_NODE_SCALE_OFFSET_Z:
    setScalingOffset(Vector3::Create(m_scalingOffset.x, m_scalingOffset.y, v));
    break;
  case KRENGINE_NODE_ATTRIBUTE_NONE:
  case KRENGINE_NODE_ATTRIBUTE_COUNT:
    // Suppress warnings
    break;
  }
}

void KRNode::setAnimationEnabled(node_attribute_type attrib, bool enable)
{
  m_animation_mask[attrib] = !enable;
}
bool KRNode::getAnimationEnabled(node_attribute_type attrib) const
{
  return !m_animation_mask[attrib];
}

void KRNode::removeFromOctreeNodes()
{
  for (std::set<KROctreeNode*>::iterator itr = m_octree_nodes.begin(); itr != m_octree_nodes.end(); itr++) {
    KROctreeNode* octree_node = *itr;
    octree_node->remove(this);

    // FINDME, TODO - This should be moved to the KROctree class
    while (octree_node) {
      octree_node->trim();
      if (octree_node->isEmpty()) {
        octree_node = octree_node->getParent();
      } else {
        octree_node = NULL;
      }
    }
  }
  m_octree_nodes.clear();
}

void KRNode::addToOctreeNode(KROctreeNode* octree_node)
{
  m_octree_nodes.insert(octree_node);
}

void KRNode::updateLODVisibility(const KRViewport& viewport)
{
  if (m_lod_visible >= LOD_VISIBILITY_PRESTREAM) {
    for (KRNode* child = m_firstChildNode; child != nullptr; child = child->m_nextNode) {
      child->updateLODVisibility(viewport);
    }
  }
}

void KRNode::setLODVisibility(KRNode::LodVisibility lod_visibility)
{
  if (m_lod_visible != lod_visibility) {
    if (m_lod_visible == LOD_VISIBILITY_HIDDEN && lod_visibility >= LOD_VISIBILITY_PRESTREAM) {
      getScene().notify_sceneGraphCreate(this);
    } else if (m_lod_visible >= LOD_VISIBILITY_PRESTREAM && lod_visibility == LOD_VISIBILITY_HIDDEN) {
      getScene().notify_sceneGraphDelete(this);
    }

    m_lod_visible = lod_visibility;

    for (KRNode* child = m_firstChildNode; child != nullptr; child = child->m_nextNode) {
      child->setLODVisibility(lod_visibility);
    }
  }
}

KRNode::LodVisibility KRNode::getLODVisibility()
{
  return m_lod_visible;
}

const Vector3 KRNode::localToWorld(const Vector3& local_point)
{
  return Matrix4::Dot(getModelMatrix(), local_point);
}

const Vector3 KRNode::worldToLocal(const Vector3& world_point)
{
  return Matrix4::Dot(getInverseModelMatrix(), world_point);
}

void KRNode::addBehavior(KRBehavior* behavior)
{
  m_behaviors.insert(behavior);
  behavior->__setNode(this);
  getScene().notify_sceneGraphModify(this);
}

std::set<KRBehavior*>& KRNode::getBehaviors()
{
  return m_behaviors;
}

kraken_stream_level KRNode::getStreamLevel(const KRViewport& viewport)
{
  kraken_stream_level stream_level = kraken_stream_level::STREAM_LEVEL_IN_HQ;

  for (KRNode* child = m_firstChildNode; child != nullptr; child = child->m_nextNode) {
    stream_level = KRMIN(stream_level, child->getStreamLevel(viewport));
  }

  return stream_level;
}

void KRNode::invalidateBounds() const
{
  m_boundsValid = false;
  if (m_parentNode) {
    m_parentNode->invalidateBounds();
  }
}
