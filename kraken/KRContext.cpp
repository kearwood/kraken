//
//  KRContext.cpp
//  Kraken Engine
//
//  Copyright 2021 Kearwood Gilbert. All rights reserved.
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

#include "KRContext.h"
#include "KRCamera.h"
#include "KRAudioManager.h"
#include "KRAudioSample.h"
#include "KRBundle.h"
#include "KRPresentationThread.h"

#if defined(ANDROID)
#include <chrono>
#include <unistd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

int KRContext::KRENGINE_MAX_PIPELINE_HANDLES;
int KRContext::KRENGINE_GPU_MEM_MAX;
int KRContext::KRENGINE_GPU_MEM_TARGET;
int KRContext::KRENGINE_MAX_TEXTURE_DIM;
int KRContext::KRENGINE_MIN_TEXTURE_DIM;
int KRContext::KRENGINE_PRESTREAM_DISTANCE;
int KRContext::KRENGINE_SYS_ALLOCATION_GRANULARITY;
int KRContext::KRENGINE_SYS_PAGE_SIZE;

#if TARGET_OS_IPHONE


#elif TARGET_OS_MAC

#elif defined(_WIN32) || defined(_WIN64)

#elif defined(ANDROID)

#else

#error Unsupported Platform

#endif

std::mutex KRContext::g_SurfaceInfoMutex;
std::mutex KRContext::g_DeviceInfoMutex;

const char *KRContext::extension_names[KRENGINE_NUM_EXTENSIONS] = {
    "GL_EXT_texture_storage"
};

KRContext::log_callback *KRContext::s_log_callback = NULL;
void *KRContext::s_log_callback_user_data = NULL;

KRContext::KRContext(const KrInitializeInfo* initializeInfo)
  : m_streamer(*this)
  , m_vulkanInstance(VK_NULL_HANDLE)
  , m_resourceMapSize(initializeInfo->resourceMapSize)
  , m_topDeviceHandle(0)
  , m_topSurfaceHandle(0)
{
    m_presentationThread = std::make_unique<KRPresentationThread>(*this);
    m_resourceMap = (KRResource **)malloc(sizeof(KRResource*) * m_resourceMapSize);
    memset(m_resourceMap, 0, m_resourceMapSize * sizeof(KRResource*));
    m_streamingEnabled = false;
#ifdef __APPLE__
    mach_timebase_info(&m_timebase_info);
#endif
    
    m_bDetectedExtensions = false;
    m_current_frame = 0;
    m_last_memory_warning_frame = 0;
    m_last_fully_streamed_frame = 0;
    m_absolute_time = 0.0f;
    
    m_pBundleManager = new KRBundleManager(*this);
    m_pPipelineManager = new KRPipelineManager(*this);
    m_pTextureManager = new KRTextureManager(*this);
    m_pMaterialManager = new KRMaterialManager(*this, m_pTextureManager, m_pPipelineManager);
    m_pMeshManager = new KRMeshManager(*this);
    m_pSceneManager = new KRSceneManager(*this);
    m_pAnimationManager = new KRAnimationManager(*this);
    m_pAnimationCurveManager = new KRAnimationCurveManager(*this);
    m_pSoundManager = new KRAudioManager(*this);
    m_pUnknownManager = new KRUnknownManager(*this);
    m_pShaderManager = new KRShaderManager(*this);
    m_pSourceManager = new KRSourceManager(*this);
    m_streamingEnabled = true;



#if defined(_WIN32) || defined(_WIN64)

    SYSTEM_INFO winSysInfo;
    GetSystemInfo(&winSysInfo);
    KRENGINE_SYS_ALLOCATION_GRANULARITY = winSysInfo.dwAllocationGranularity;
    KRENGINE_SYS_PAGE_SIZE = winSysInfo.dwPageSize;

#elif defined(__APPLE__) || defined(ANDROID)

    KRENGINE_SYS_PAGE_SIZE = getpagesize();
    KRENGINE_SYS_ALLOCATION_GRANULARITY = KRENGINE_SYS_PAGE_SIZE;

#else
#error Unsupported
#endif
    
    createDeviceContexts();
    m_presentationThread->start();
}

KRContext::~KRContext() {
    m_presentationThread->stop();
    if(m_pSceneManager) {
        delete m_pSceneManager;
        m_pSceneManager = NULL;
    }
    
    if(m_pMeshManager) {
        delete m_pMeshManager;
        m_pMeshManager = NULL;
    }
    
    if(m_pTextureManager) {
        delete m_pTextureManager;
        m_pTextureManager = NULL;
    }
    
    if(m_pMaterialManager) {
        delete m_pMaterialManager;
        m_pMaterialManager = NULL;
    }
    
    if(m_pPipelineManager) {
        delete m_pPipelineManager;
        m_pPipelineManager = NULL;
    }
    
    if(m_pAnimationManager) {
        delete m_pAnimationManager;
        m_pAnimationManager = NULL;
    }
    
    if(m_pAnimationCurveManager) {
        delete m_pAnimationCurveManager;
        m_pAnimationCurveManager = NULL;
    }
    
    if(m_pSoundManager) {
        delete m_pSoundManager;
        m_pSoundManager = NULL;
    }

    if(m_pSourceManager) {
        delete m_pSourceManager;
        m_pSourceManager = NULL;
    }

    if(m_pUnknownManager) {
        delete m_pUnknownManager;
        m_pUnknownManager = NULL;
    }
    
    // The bundles must be destroyed last, as the other objects may be using mmap'ed data from bundles
    if(m_pBundleManager) {
        delete m_pBundleManager;
        m_pBundleManager = NULL;
    }
    destroySurfaces();
    destroyDeviceContexts();
    if (m_resourceMap) {
        delete m_resourceMap;
        m_resourceMap = NULL;
    }
}

void KRContext::SetLogCallback(log_callback *log_callback, void *user_data)
{
    s_log_callback = log_callback;
    s_log_callback_user_data = user_data;
}

void KRContext::Log(log_level level, const std::string message_format, ...)
{
    va_list args;
    va_start(args, message_format);
    
    if(s_log_callback) {
        const int LOG_BUFFER_SIZE = 32768;
        char log_buffer[LOG_BUFFER_SIZE];
        vsnprintf(log_buffer, LOG_BUFFER_SIZE, message_format.c_str(), args);
        s_log_callback(s_log_callback_user_data, std::string(log_buffer), level);
    } else {
        FILE *out_file = level == LOG_LEVEL_INFORMATION ? stdout : stderr;
        fprintf(out_file, "Kraken - INFO: ");
        vfprintf(out_file, message_format.c_str(), args);
        fprintf(out_file, "\n");
    }
    
    va_end(args);
}

KRBundleManager *KRContext::getBundleManager() {
    return m_pBundleManager;
}
KRSceneManager *KRContext::getSceneManager() {
    return m_pSceneManager;
}
KRTextureManager *KRContext::getTextureManager() {
    return m_pTextureManager;
}
KRMaterialManager *KRContext::getMaterialManager() {
    return m_pMaterialManager;
}
KRPipelineManager *KRContext::getPipelineManager() {
    return m_pPipelineManager;
}
KRMeshManager *KRContext::getMeshManager() {
    return m_pMeshManager;
}
KRAnimationManager *KRContext::getAnimationManager() {
    return m_pAnimationManager;
}
KRAnimationCurveManager *KRContext::getAnimationCurveManager() {
    return m_pAnimationCurveManager;
}
KRAudioManager *KRContext::getAudioManager() {
    return m_pSoundManager;
}
KRShaderManager *KRContext::getShaderManager() {
    return m_pShaderManager;
}
KRSourceManager *KRContext::getSourceManager() {
    return m_pSourceManager;
}
KRUnknownManager *KRContext::getUnknownManager() {
    return m_pUnknownManager;
}
std::vector<KRResource *> KRContext::getResources()
{
    std::vector<KRResource *> resources;
    
    for(unordered_map<std::string, KRScene *>::iterator itr = m_pSceneManager->getScenes().begin(); itr != m_pSceneManager->getScenes().end(); itr++) {
        resources.push_back((*itr).second);
    }
    for(unordered_map<std::string, KRTexture *>::iterator itr = m_pTextureManager->getTextures().begin(); itr != m_pTextureManager->getTextures().end(); itr++) {
        resources.push_back((*itr).second);
    }
    for(unordered_map<std::string, KRMaterial *>::iterator itr = m_pMaterialManager->getMaterials().begin(); itr != m_pMaterialManager->getMaterials().end(); itr++) {
        resources.push_back((*itr).second);
    }
    for(unordered_multimap<std::string, KRMesh *>::iterator itr = m_pMeshManager->getModels().begin(); itr != m_pMeshManager->getModels().end(); itr++) {
        resources.push_back((*itr).second);
    }
    for(unordered_map<std::string, KRAnimation *>::iterator itr = m_pAnimationManager->getAnimations().begin(); itr != m_pAnimationManager->getAnimations().end(); itr++) {
        resources.push_back((*itr).second);
    }
    for(unordered_map<std::string, KRAnimationCurve *>::iterator itr = m_pAnimationCurveManager->getAnimationCurves().begin(); itr != m_pAnimationCurveManager->getAnimationCurves().end(); itr++) {
        resources.push_back((*itr).second);
    }
    for(unordered_map<std::string, KRAudioSample *>::iterator itr = m_pSoundManager->getSounds().begin(); itr != m_pSoundManager->getSounds().end(); itr++) {
        resources.push_back((*itr).second);
    }

    unordered_map<std::string, unordered_map<std::string, KRSource *> > sources = m_pSourceManager->getSources();
    for(unordered_map<std::string, unordered_map<std::string, KRSource *> >::iterator itr = sources.begin(); itr != sources.end(); itr++) {
        for(unordered_map<std::string, KRSource *>::iterator itr2 = (*itr).second.begin(); itr2 != (*itr).second.end(); itr2++) {
            resources.push_back((*itr2).second);
        }
    }

    unordered_map<std::string, unordered_map<std::string, KRShader *> > shaders = m_pShaderManager->getShaders();
    for(unordered_map<std::string, unordered_map<std::string, KRShader *> >::iterator itr = shaders.begin(); itr != shaders.end(); itr++) {
        for(unordered_map<std::string, KRShader *>::iterator itr2 = (*itr).second.begin(); itr2 != (*itr).second.end(); itr2++) {
            resources.push_back((*itr2).second);
        }
    }
    
    unordered_map<std::string, unordered_map<std::string, KRUnknown *> > unknowns = m_pUnknownManager->getUnknowns();
    for(unordered_map<std::string, unordered_map<std::string, KRUnknown *> >::iterator itr = unknowns.begin(); itr != unknowns.end(); itr++) {
        for(unordered_map<std::string, KRUnknown *>::iterator itr2 = (*itr).second.begin(); itr2 != (*itr).second.end(); itr2++) {
            resources.push_back((*itr2).second);
        }
    }
    
    return resources;
}

KRResource* KRContext::loadResource(const std::string &file_name, KRDataBlock *data) {
    std::string name = KRResource::GetFileBase(file_name);
    std::string extension = KRResource::GetFileExtension(file_name);

    KRResource *resource = nullptr;
    
//    fprintf(stderr, "KRContext::loadResource - Loading: %s\n", file_name.c_str());
    
    if(extension.compare("krbundle") == 0) {
        resource = m_pBundleManager->loadBundle(name.c_str(), data);
    } else if(extension.compare("krmesh") == 0) {
        resource = m_pMeshManager->loadModel(name.c_str(), data);
    } else if(extension.compare("krscene") == 0) {
        resource = m_pSceneManager->loadScene(name.c_str(), data);
    } else if(extension.compare("kranimation") == 0) {
        resource = m_pAnimationManager->loadAnimation(name.c_str(), data);
    } else if(extension.compare("kranimationcurve") == 0) {
        resource = m_pAnimationCurveManager->loadAnimationCurve(name.c_str(), data);
    } else if(extension.compare("pvr") == 0) {
        resource = m_pTextureManager->loadTexture(name.c_str(), extension.c_str(), data);
    } else if(extension.compare("ktx") == 0) {
        resource = m_pTextureManager->loadTexture(name.c_str(), extension.c_str(), data);
    } else if(extension.compare("tga") == 0) {
        resource = m_pTextureManager->loadTexture(name.c_str(), extension.c_str(), data);
    } else if(extension.compare("spv") == 0) {
        // SPIR-V shader binary
        resource = m_pShaderManager->load(name, extension, data);
    } else if(extension.compare("vert") == 0) {
        // vertex shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("frag") == 0) {
        // fragment shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("tesc") == 0) {
        // tessellation control shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("tese") == 0) {
        // tessellation evaluation shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("geom") == 0) {
        // geometry shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("comp") == 0) {
        // compute shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("mesh") == 0) {
        // mesh shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("task") == 0) {
        // task shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("rgen") == 0) {
        // ray generation shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("rint") == 0) {
        // ray intersection shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("rahit") == 0) {
        // ray any hit shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("rchit") == 0) {
        // ray closest hit shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("rmiss") == 0) {
        // ray miss shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("rcall") == 0) {
        // ray callable shader
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("glsl") == 0) {
        // glsl included by other shaders
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("options") == 0) {
        // shader pre-processor options definition file
        resource = m_pSourceManager->load(name, extension, data);
    } else if(extension.compare("mtl") == 0) {
        resource = m_pMaterialManager->load(name.c_str(), data);
    } else if(extension.compare("mp3") == 0) {
        resource = m_pSoundManager->load(name.c_str(), extension, data);
    } else if(extension.compare("wav") == 0) {
        resource = m_pSoundManager->load(name.c_str(), extension, data);
    } else if(extension.compare("aac") == 0) {
        resource = m_pSoundManager->load(name.c_str(), extension, data);
    } else if(extension.compare("obj") == 0) {
        resource = KRResource::LoadObj(*this, file_name);
#if !TARGET_OS_IPHONE
/*
  // FINDME, TODO, HACK! - Uncomment
    } else if(extension.compare("fbx") == 0) {
        resource = KRResource::LoadFbx(*this, file_name);
*/
    } else if(extension.compare("blend") == 0) {
        resource = KRResource::LoadBlenderScene(*this, file_name);
#endif
    } else {
        resource = m_pUnknownManager->load(name, extension, data);
    }
    return resource;
}

KrResult KRContext::loadResource(const KrLoadResourceInfo* loadResourceInfo) {
    if (loadResourceInfo->resourceHandle < 0 || loadResourceInfo->resourceHandle >= m_resourceMapSize) {
      return KR_ERROR_OUT_OF_BOUNDS;
    }
    KRDataBlock *data = new KRDataBlock();
    if(!data->load(loadResourceInfo->pResourcePath)) {
      KRContext::Log(KRContext::LOG_LEVEL_ERROR, "KRContext::loadResource - Failed to open file: %s", loadResourceInfo->pResourcePath);
      delete data;
      return KR_ERROR_UNEXPECTED;
    }

    KRResource *resource = loadResource(loadResourceInfo->pResourcePath, data);
    m_resourceMap[loadResourceInfo->resourceHandle] = resource;
    return KR_SUCCESS;
}

KrResult KRContext::unloadResource(const KrUnloadResourceInfo* unloadResourceInfo)
{
  if (unloadResourceInfo->resourceHandle < 0 || unloadResourceInfo->resourceHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  KRResource* resource = m_resourceMap[unloadResourceInfo->resourceHandle];
  if (resource == nullptr) {
    return KR_ERROR_NOT_MAPPED;
  }
  // TODO - Need to implement unloading logic
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::mapResource(const KrMapResourceInfo* mapResourceInfo)
{
  if (mapResourceInfo->resourceHandle < 0 || mapResourceInfo->resourceHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }

  std::string lowerName = mapResourceInfo->pResourceName;
  std::transform(lowerName.begin(), lowerName.end(),
    lowerName.begin(), ::tolower);

  KRResource* resource = nullptr;

  std::pair<unordered_multimap<std::string, KRResource*>::iterator, unordered_multimap<std::string, KRResource*>::iterator> range = m_resources.equal_range(lowerName);
  for (unordered_multimap<std::string, KRResource*>::iterator itr_match = range.first; itr_match != range.second; itr_match++) {
    if (resource != nullptr) {
      return KR_ERROR_AMBIGUOUS_MATCH;
    }
    resource = itr_match->second;
  }
  if (resource == nullptr) {
    return KR_ERROR_NOT_FOUND;
  }
  m_resourceMap[mapResourceInfo->resourceHandle] = resource;
  return KR_SUCCESS;
}

KrResult KRContext::unmapResource(const KrUnmapResourceInfo* unmapResourceInfo)
{
  if (unmapResourceInfo->resourceHandle < 0 || unmapResourceInfo->resourceHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  m_resourceMap[unmapResourceInfo->resourceHandle] = nullptr;
  // TODO - Delete objects after lass dereference
  return KR_SUCCESS;
}

KrResult KRContext::getResourceData(const KrGetResourceDataInfo* getResourceDataInfo, KrGetResourceDataCallback callback)
{
  if (getResourceDataInfo->resourceHandle < 0 || getResourceDataInfo->resourceHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  // TODO - This will be asynchronous...
  KRDataBlock data;
  KrGetResourceDataResult result = {};
  if (m_resourceMap[getResourceDataInfo->resourceHandle] == nullptr) {
    result.result = KR_ERROR_NOT_MAPPED;
    callback(result);
  } else if (m_resourceMap[getResourceDataInfo->resourceHandle]->save(data)) {
    data.lock();
    result.data = data.getStart();
    result.length = static_cast<size_t>(data.getSize());
    result.result = KR_SUCCESS;
    callback(result);
    data.unlock();
  } else {
    result.result = KR_ERROR_UNEXPECTED;
    callback(result);
  }
 
  return KR_SUCCESS;
}

KrResult KRContext::createScene(const KrCreateSceneInfo* createSceneInfo)
{
  if (createSceneInfo->resourceHandle < 0 || createSceneInfo->resourceHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  KRScene* scene = m_pSceneManager->createScene(createSceneInfo->pSceneName);
  m_resourceMap[createSceneInfo->resourceHandle] = scene;
  return KR_SUCCESS;
}

KrResult KRContext::createBundle(const KrCreateBundleInfo* createBundleInfo)
{
  if (createBundleInfo->resourceHandle < 0 || createBundleInfo->resourceHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  KRResource* bundle = m_pBundleManager->createBundle(createBundleInfo->pBundleName);
  m_resourceMap[createBundleInfo->resourceHandle] = bundle;

  return KR_SUCCESS;
}

KrResult KRContext::moveToBundle(const KrMoveToBundleInfo* moveToBundleInfo)
{
  if (moveToBundleInfo->bundleHandle < 0 || moveToBundleInfo->bundleHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  if (moveToBundleInfo->resourceHandle < 0 || moveToBundleInfo->resourceHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  KRResource* resource = m_resourceMap[moveToBundleInfo->resourceHandle];
  if (resource == nullptr) {
    return KR_ERROR_NOT_MAPPED;
  }
  KRResource* bundleResource = m_resourceMap[moveToBundleInfo->bundleHandle];
  if (bundleResource == nullptr) {
    return KR_ERROR_NOT_MAPPED;
  }
  KRBundle* bundle = dynamic_cast<KRBundle*>(bundleResource);
  if (bundle == nullptr) {
    return KR_ERROR_INCORRECT_TYPE;
  }
  return resource->moveToBundle(bundle);
}

KrResult KRContext::compileAllShaders(const KrCompileAllShadersInfo* pCompileAllShadersInfo)
{
  if (pCompileAllShadersInfo->bundleHandle < 0 || pCompileAllShadersInfo->bundleHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  KRResource* bundleResource = m_resourceMap[pCompileAllShadersInfo->bundleHandle];
  if (bundleResource == nullptr) {
    return KR_ERROR_NOT_MAPPED;
  }
  KRBundle* bundle = dynamic_cast<KRBundle*>(bundleResource);
  if (bundle == nullptr) {
    return KR_ERROR_INCORRECT_TYPE;
  }
  if (pCompileAllShadersInfo->logHandle < -1 || pCompileAllShadersInfo->logHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }

  KRResource* existing_log = m_pUnknownManager->getResource("shader_compile", "log");
  KRUnknown* logResource = nullptr;
  if (existing_log != nullptr) {
    logResource = dynamic_cast<KRUnknown*>(existing_log);
  }
  if (logResource == nullptr) {
    logResource = new KRUnknown(*this, "shader_compile", "log");
    m_pUnknownManager->add(logResource);
  }
  if (pCompileAllShadersInfo->logHandle != -1) {
    m_resourceMap[pCompileAllShadersInfo->logHandle] = logResource;
  }

  bool success = m_pShaderManager->compileAll(bundle, logResource);
  if (success) {
    return KR_SUCCESS;
  }
  return KR_ERROR_SHADER_COMPILE_FAILED;
}

KrResult KRContext::saveResource(const KrSaveResourceInfo* saveResourceInfo)
{
  if (saveResourceInfo->resourceHandle < 0 || saveResourceInfo->resourceHandle >= m_resourceMapSize) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  KRResource* resource = m_resourceMap[saveResourceInfo->resourceHandle];
  if (resource == nullptr) {
    return KR_ERROR_NOT_MAPPED;
  }
  if (resource->save(saveResourceInfo->pResourcePath)) {
    return KR_SUCCESS;
  }
  return KR_ERROR_UNEXPECTED;
}

void KRContext::detectExtensions() {
    m_bDetectedExtensions = true;
    
}

void KRContext::startFrame(float deltaTime)
{
    m_streamer.startStreamer();
    m_pTextureManager->startFrame(deltaTime);
    m_pAnimationManager->startFrame(deltaTime);
    m_pSoundManager->startFrame(deltaTime);
    m_pMeshManager->startFrame(deltaTime);
}

void KRContext::endFrame(float deltaTime)
{
    m_pTextureManager->endFrame(deltaTime);
    m_pAnimationManager->endFrame(deltaTime);
    m_pMeshManager->endFrame(deltaTime);
    m_current_frame++;
    m_absolute_time += deltaTime;
}

long KRContext::getCurrentFrame() const
{
    return m_current_frame;
}

long KRContext::getLastFullyStreamedFrame() const
{
    return m_last_fully_streamed_frame;
}

float KRContext::getAbsoluteTime() const
{
    return m_absolute_time;
}


long KRContext::getAbsoluteTimeMilliseconds()
{
#if defined(ANDROID)
    return std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()).count();
#elif defined(__APPLE__)
    return (long)(mach_absolute_time() / 1000 * m_timebase_info.numer / m_timebase_info.denom); // Division done first to avoid potential overflow
#else
    return (long)GetTickCount64();
#endif
}

bool KRContext::getStreamingEnabled()
{
    return m_streamingEnabled;
}

void KRContext::setStreamingEnabled(bool enable)
{
    m_streamingEnabled = enable;
}


#if TARGET_OS_IPHONE || TARGET_OS_MAC

void KRContext::getMemoryStats(long &free_memory)
{
    free_memory = 0;

    mach_port_t host_port = mach_host_self();
    mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
    vm_size_t pagesize = 0;
    vm_statistics_data_t vm_stat;
    // int total_ram = 256 * 1024 * 1024;
    if(host_page_size(host_port, &pagesize) != KERN_SUCCESS) {
        KRContext::Log(KRContext::LOG_LEVEL_ERROR, "Could not get VM page size.");
    } else if(host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS) {
        KRContext::Log(KRContext::LOG_LEVEL_ERROR, "Could not get VM stats.");
    } else {
        // total_ram = (vm_stat.wire_count + vm_stat.active_count + vm_stat.inactive_count + vm_stat.free_count) * pagesize;
        
        free_memory = (vm_stat.free_count + vm_stat.inactive_count) * pagesize;
    }
}

#endif

void KRContext::doStreaming()
{
  if (m_streamingEnabled) {
    /*
    long free_memory = KRENGINE_GPU_MEM_TARGET;
    long total_memory = KRENGINE_GPU_MEM_MAX;
    */
    /*
#if TARGET_OS_IPHONE
        // FINDME, TODO, HACK! - Experimental code, need to expose through engine parameters
        const long KRENGINE_RESERVE_MEMORY = 0x4000000; // 64MB

        getMemoryStats(free_memory);
        free_memory = KRCLAMP(free_memory - KRENGINE_RESERVE_MEMORY, 0, KRENGINE_GPU_MEM_TARGET);
        total_memory = KRMIN(KRENGINE_GPU_MEM_MAX, free_memory * 3 / 4 + m_pTextureManager->getMemUsed() + m_pMeshManager->getMemUsed());

#endif
        */
        /*
        // FINDME, TODO - Experimental code, need to expose through engine parameters
        const long MEMORY_WARNING_THROTTLE_FRAMES = 5;
        bool memory_warning_throttle = m_last_memory_warning_frame != 0 && m_current_frame - m_last_memory_warning_frame < MEMORY_WARNING_THROTTLE_FRAMES;
        if(memory_warning_throttle) {
            free_memory = 0;
        }
        */

        /*
        // FINDME, TODO - Experimental code, need to expose through engine parameters
        const long MEMORY_WARNING_THROTTLE2_FRAMES = 30;
        bool memory_warning_throttle2 = m_last_memory_warning_frame != 0 && m_current_frame - m_last_memory_warning_frame < MEMORY_WARNING_THROTTLE2_FRAMES;
        if(memory_warning_throttle2) {
            total_memory /= 2;
            free_memory /= 2;
        }
        */

        /*
        m_pMeshManager->doStreaming(total_memory, free_memory);
        m_pTextureManager->doStreaming(total_memory, free_memory);
        */


    long streaming_start_frame = m_current_frame;

    long memoryRemaining = KRENGINE_GPU_MEM_TARGET;
    long memoryRemainingThisFrame = KRENGINE_GPU_MEM_MAX - m_pTextureManager->getMemUsed() - m_pMeshManager->getMemUsed();
    long memoryRemainingThisFrameStart = memoryRemainingThisFrame;
    m_pMeshManager->doStreaming(memoryRemaining, memoryRemainingThisFrame);
    m_pTextureManager->doStreaming(memoryRemaining, memoryRemainingThisFrame);

    if (memoryRemainingThisFrame == memoryRemainingThisFrameStart && memoryRemainingThisFrame > 0) {
      m_last_fully_streamed_frame = streaming_start_frame;
    }

  }
}

void KRContext::receivedMemoryWarning()
{
  m_last_memory_warning_frame = m_current_frame;
}

void
KRContext::createDeviceContexts()
{
  VkResult res = volkInitialize();
  if (res != VK_SUCCESS) {
    destroyDeviceContexts();
    return;
  }

  // initialize the VkApplicationInfo structure
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pNext = NULL;
  app_info.pApplicationName = "Test"; // TODO - Change Me!
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.pEngineName = "Kraken Engine";
  app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  // VK_KHR_surface and VK_KHR_win32_surface

  char* extensions[] = {
    "VK_KHR_surface",
#ifdef WIN32
    "VK_KHR_win32_surface",
#endif
  };

  // initialize the VkInstanceCreateInfo structure
  VkInstanceCreateInfo inst_info = {};
  inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  inst_info.pNext = NULL;
  inst_info.flags = 0;
  inst_info.pApplicationInfo = &app_info;
#ifdef WIN32
  inst_info.enabledExtensionCount = 2;
#else
  inst_info.enabledExtensionCount = 1;
#endif
  inst_info.ppEnabledExtensionNames = extensions;
  inst_info.enabledLayerCount = 0;
  inst_info.ppEnabledLayerNames = NULL;

  res = vkCreateInstance(&inst_info, NULL, &m_vulkanInstance);
  if (res != VK_SUCCESS) {
    destroyDeviceContexts();
    return;
  }

  volkLoadInstance(m_vulkanInstance);

  createDevices();
}

void
KRContext::destroyDeviceContexts()
{
  const std::lock_guard<std::mutex> lock(KRContext::g_DeviceInfoMutex);
  m_devices.clear();
  if (m_vulkanInstance != VK_NULL_HANDLE) {
    vkDestroyInstance(m_vulkanInstance, NULL);
    m_vulkanInstance = VK_NULL_HANDLE;
  }
}

void
KRContext::destroySurfaces()
{
  const std::lock_guard<std::mutex> surfaceLock(KRContext::g_SurfaceInfoMutex);
  m_surfaces.clear();
  m_surfaceHandleMap.clear();
}

void
KRContext::activateStreamerContext()
{

}

KrResult KRContext::findNodeByName(const KrFindNodeByNameInfo* pFindNodeByNameInfo)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::findAdjacentNodes(const KrFindAdjacentNodesInfo* pFindAdjacentNodesInfo)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::setNodeLocalTransform(const KrSetNodeLocalTransformInfo* pSetNodeLocalTransform)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::setNodeWorldTransform(const KrSetNodeWorldTransformInfo* pSetNodeWorldTransform)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::deleteNode(const KrDeleteNodeInfo* pDeleteNodeInfo)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::deleteNodeChildren(const KrDeleteNodeChildrenInfo* pDeleteNodeChildrenInfo)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::appendBeforeNode(const KrAppendBeforeNodeInfo* pAppendBeforeNodeInfo)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::appendAfterNode(const KrAppendAfterNodeInfo* pAppendAfterNodeInfo)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::appendFirstChildNode(const KrAppendFirstChildNodeInfo* pAppendFirstChildNodeInfo)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::appendLastChildNode(const KrAppendLastChildNodeInfo* pAppendLastChildNodeInfo)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

KrResult KRContext::updateNode(const KrUpdateNodeInfo* pUpdateNodeInfo)
{
  return KR_ERROR_NOT_IMPLEMENTED;
}

void KRContext::addResource(KRResource* resource, const std::string& name)
{
  std::string lowerName = name;
  std::transform(lowerName.begin(), lowerName.end(),
    lowerName.begin(), ::tolower);

  m_resources.insert(std::pair<std::string, KRResource*>(lowerName, resource));
}

void KRContext::removeResource(KRResource* resource)
{
  std::string lowerName = resource->getName();
  std::transform(lowerName.begin(), lowerName.end(),
    lowerName.begin(), ::tolower);

  std::pair<unordered_multimap<std::string, KRResource*>::iterator, unordered_multimap<std::string, KRResource*>::iterator> range = m_resources.equal_range(lowerName);
  for (unordered_multimap<std::string, KRResource*>::iterator itr_match = range.first; itr_match != range.second; itr_match++) {
    if (itr_match->second == resource) {
      m_resources.erase(itr_match);
      return;
    }
  }
}

KrSurfaceHandle KRContext::GetBestDeviceForSurface(const VkSurfaceKHR& surface)
{
  KrDeviceHandle deviceHandle = 0;
  for (auto itr = m_devices.begin(); itr != m_devices.end(); itr++) {
    KRDevice& device = *(*itr).second;
    VkBool32 canPresent = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device.m_device, device.m_graphicsFamilyQueueIndex, surface, &canPresent);
    if (canPresent) {
      deviceHandle = (*itr).first;
      break;
    }
  }
  return deviceHandle;
}

KrResult KRContext::createWindowSurface(const KrCreateWindowSurfaceInfo* createWindowSurfaceInfo)
{
  if (createWindowSurfaceInfo->surfaceHandle < 0) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  if (m_vulkanInstance == VK_NULL_HANDLE) {
    return KR_ERROR_VULKAN_REQUIRED;
  }
  if (m_surfaceHandleMap.count(createWindowSurfaceInfo->surfaceHandle)) {
    return KR_ERROR_DUPLICATE_HANDLE;
  }

  if (m_devices.size() == 0) {
    return KR_ERROR_NO_DEVICE;
  }

  const std::lock_guard<std::mutex> surfaceLock(KRContext::g_SurfaceInfoMutex);
  const std::lock_guard<std::mutex> deviceLock(KRContext::g_DeviceInfoMutex);

#ifdef WIN32
  HWND hWnd = static_cast<HWND>(createWindowSurfaceInfo->hWnd);
  std::unique_ptr<KRSurface> info = std::make_unique<KRSurface>(*this, hWnd);

  KrResult initialize_result = info->initialize();
  if (initialize_result != KR_SUCCESS) {
    return initialize_result;
  }

  KRDevice* deviceInfo  = &GetDeviceInfo(info->m_deviceHandle);

  KrSurfaceHandle surfaceHandle = ++m_topSurfaceHandle;
  m_surfaces.insert(std::pair<KrSurfaceHandle, std::unique_ptr<KRSurface>>(surfaceHandle, std::move(info)));
  
  m_surfaceHandleMap.insert(std::pair<KrSurfaceMapIndex, KrSurfaceHandle>(createWindowSurfaceInfo->surfaceHandle, surfaceHandle));

  return KR_SUCCESS;
#else
  // Not implemented for this platform
  return KR_ERROR_NOT_IMPLEMENTED;
#endif
}

KrResult KRContext::deleteWindowSurface(const KrDeleteWindowSurfaceInfo* deleteWindowSurfaceInfo)
{
  if (deleteWindowSurfaceInfo->surfaceHandle < 0) {
    return KR_ERROR_OUT_OF_BOUNDS;
  }
  if (m_vulkanInstance == VK_NULL_HANDLE) {
    return KR_ERROR_VULKAN_REQUIRED;
  }

  auto handleItr = m_surfaceHandleMap.find(deleteWindowSurfaceInfo->surfaceHandle);
  if (handleItr == m_surfaceHandleMap.end()) {
    return KR_ERROR_NOT_FOUND;
  }
  KrSurfaceHandle surfaceHandle = (*handleItr).second;
  m_surfaceHandleMap.erase(handleItr);
  
  auto itr = m_surfaces.find(surfaceHandle);
  if (itr == m_surfaces.end()) {
    return KR_ERROR_NOT_FOUND;
  }
  m_surfaces.erase(itr);
  return KR_SUCCESS;
}

KRSurface& KRContext::GetSurfaceInfo(KrSurfaceHandle handle)
{
  auto itr = m_surfaces.find(handle);
  if (itr == m_surfaces.end()) {
    assert(false);
  }
  return *m_surfaces[handle];
}

KRDevice& KRContext::GetDeviceInfo(KrDeviceHandle handle)
{
  return *m_devices[handle];
}

void KRContext::createDevices()
{
  const std::lock_guard<std::mutex> deviceLock(KRContext::g_DeviceInfoMutex);
  if (m_devices.size() > 0) {
    return;
  }
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    return;
  }

  std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
  vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, physicalDevices.data());

  const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  std::vector<std::unique_ptr<KRDevice>> candidateDevices;

  for (const VkPhysicalDevice& physicalDevice : physicalDevices) {
    std::unique_ptr<KRDevice> device = std::make_unique<KRDevice>(physicalDevice);
    if (!device->initialize(deviceExtensions)) {
      continue;
    }

    bool addDevice = false;
    if (candidateDevices.empty()) {
      addDevice = true;
    } else {
      VkPhysicalDeviceType collectedType = candidateDevices[0]->m_deviceProperties.deviceType;
      if (collectedType == device->m_deviceProperties.deviceType) {
        addDevice = true;
      } else if (device->m_deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        // Discrete GPU's are always the best choice
        candidateDevices.clear();
        addDevice = true;
      } else if (collectedType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device->m_deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        // Integrated GPU's are the second best choice
        candidateDevices.clear();
        addDevice = true;
      } else if (collectedType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && collectedType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && device->m_deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
        // Virtual GPU's are the 3rd best choice
        candidateDevices.clear();
        addDevice = true;
      }
    }
    if (addDevice) {
      candidateDevices.push_back(std::move(device));
    }
  }

  for (auto itr = candidateDevices.begin(); itr != candidateDevices.end(); itr++) {
    std::unique_ptr<KRDevice> device = std::move(*itr);
     m_devices[++m_topDeviceHandle] = std::move(device);
  }
}

VkInstance& KRContext::GetVulkanInstance()
{
  return m_vulkanInstance;
}

unordered_map<KrSurfaceHandle, std::unique_ptr<KRSurface>>& KRContext::GetSurfaces()
{
  return m_surfaces;
}
