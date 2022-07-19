//
//  KRDevice.h
//  Kraken Engine
//
//  Copyright 2022 Kearwood Gilbert. All rights reserved.
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
#include "KRContextObject.h"

#pragma once

class KRDataBlock;

class KRDevice : public KRContextObject
{
public:
  KRDevice(KRContext& context, const VkPhysicalDevice& device);
  virtual ~KRDevice();

  KRDevice(const KRDevice&) = delete;
  KRDevice& operator=(const KRDevice&) = delete;

  void destroy();
  bool initialize(const std::vector<const char*>& deviceExtensions);

  VmaAllocator getAllocator();
  bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VmaAllocation* allocation
#if KRENGINE_DEBUG_GPU_LABELS  
    , const char* debug_label
#endif
  );

  bool createImage(Vector2i dimensions, VkImageCreateFlags imageCreateFlags, VkMemoryPropertyFlags properties, VkImage* image, VmaAllocation* allocation);

  KrResult selectSurfaceFormat(VkSurfaceKHR& surface, VkSurfaceFormatKHR& surfaceFormat);
  KrResult selectDepthFormat(VkFormat& selectedDepthFormat);
  KrResult selectPresentMode(VkSurfaceKHR& surface, VkPresentModeKHR& selectedPresentMode);

  void streamStart();
  size_t streamRemaining() const;
  void streamUpload(KRDataBlock& data, VkBuffer destination);
  void streamUpload(void *data, size_t size, VkBuffer destination);
  void streamUpload(void* data, size_t size, Vector2i dimensions, VkImage destination);
  void streamEnd();

  VkPhysicalDevice m_device;
  VkDevice m_logicalDevice;
  VkPhysicalDeviceProperties m_deviceProperties;
  VkPhysicalDeviceFeatures m_deviceFeatures;
  uint32_t m_graphicsFamilyQueueIndex;
  VkQueue m_graphicsQueue;
  uint32_t m_computeFamilyQueueIndex;
  VkQueue m_computeQueue;
  uint32_t m_transferFamilyQueueIndex;
  VkQueue m_transferQueue;
  VkCommandPool m_graphicsCommandPool;
  VkCommandPool m_computeCommandPool;
  VkCommandPool m_transferCommandPool;
  std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
  std::vector<VkCommandBuffer> m_computeCommandBuffers;
  std::vector<VkCommandBuffer> m_transferCommandBuffers;
  VmaAllocator m_allocator;

  // Staging buffer for uploading with the transfer queue
  // This will be used for asynchronous asset streaming in the streamer thread.
  // TODO - We should allocate at least two of these and double-buffer for increased CPU-GPU concurrency
  VkBuffer m_streamingStagingBuffer;
  VmaAllocation m_streamingStagingBufferAllocation;
  size_t m_streamingStagingBufferSize;
  size_t m_streamingStagingBufferUsage;
  void* m_streamingStagingBufferData;

  // Staging buffer for uploading with the graphics queue
  // This will be used for uploading assets procedurally generated while recording the graphics command buffer.
  // TODO - We should allocate at least two of these and double-buffer for increased CPU-GPU concurrency
  VkBuffer m_graphicsStagingBuffer;
  VmaAllocation m_graphicsStagingBufferAllocation;
  size_t m_graphicsStagingBufferSize;
  size_t m_graphicsStagingBufferUsage;
  void* m_graphicsStagingBufferData;
private:

  void getQueueFamiliesForSharing(uint32_t* queueFamilyIndices, uint32_t* familyCount);

  // Initialization helper functions
  bool getAndCheckDeviceCapabilities(const std::vector<const char*>& deviceExtensions);
  bool selectQueueFamilies();
  bool initDeviceAndQueues(const std::vector<const char*>& deviceExtensions);
  bool initCommandPools();
  bool initCommandBuffers();
  bool initAllocator();
  bool initStagingBuffers();

  bool createStagingBuffer(VkDeviceSize size, VkBuffer* buffer, VmaAllocation* allocation, void** data
#if KRENGINE_DEBUG_GPU_LABELS
    , const char* debug_label
#endif // KRENGINE_DEBUG_GPU_LABELS
  );
};
