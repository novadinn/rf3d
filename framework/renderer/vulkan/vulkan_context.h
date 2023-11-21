#pragma once

#include "vulkan_command_buffer.h"
#include "vulkan_descriptor_layout_cache.h"
#include "vulkan_descriptor_pools.h"
#include "vulkan_device.h"
#include "vulkan_fence.h"
#include "vulkan_swapchain.h"

#include "vk_mem_alloc.h"
#include <assert.h>
#include <vector>
#include <vulkan/vulkan.h>

/* TODO: get rid of that macro and handle errors by our own */
#define VK_CHECK(result)                                                       \
  { assert(result == VK_SUCCESS); }

class VulkanContext {
public:
  VkInstance instance;
  VmaAllocator vma_allocator;
  VkAllocationCallbacks *allocator;
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debug_messenger;
#endif
  VkSurfaceKHR surface;
  VulkanDevice *device;
  VulkanSwapchain *swapchain;
  std::vector<VkSemaphore> image_available_semaphores;
  std::vector<VkSemaphore> queue_complete_semaphores;
  std::vector<VulkanFence *> in_flight_fences;
  std::vector<VulkanFence *> images_in_flight;

  uint32_t image_index;
  uint32_t current_frame;

  /* TODO: VkPipelineCache */
  VulkanDescriptorPools *descriptor_pools;
  VulkanDescriptorLayoutCache *layout_cache;
};