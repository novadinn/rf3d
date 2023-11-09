#pragma once

#include "vulkan_command_buffer.h"

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

enum VulkanDeviceQueueType {
  VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS,
  VULKAN_DEVICE_QUEUE_TYPE_PRESENT,
  VULKAN_DEVICE_QUEUE_TYPE_TRANSFER,
  /* TODO: others */
};

struct VulkanPhysicalDeviceRequirements {
  std::vector<const char *> device_extension_names;
  bool graphics;
  bool present;
  bool transfer;
};

struct VulkanSwapchainSupportInfo {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

/* queue family specific info */
struct VulkanDeviceQueueInfo {
  int family_index;
  VkQueue queue;
  VkCommandPool command_pool;
  std::vector<VulkanCommandBuffer> command_buffers;
};

class VulkanDevice {
public:
  bool Create(VulkanPhysicalDeviceRequirements *requirements);
  void Destroy();

  void UpdateSwapchainSupport();
  void UpdateDepthFormat();
  void UpdateCommandBuffers();

  inline VkPhysicalDevice GetPhysicalDevice() const { return physical_device; }
  inline VkDevice GetLogicalDevice() const { return logical_device; }
  inline VulkanDeviceQueueInfo GetQueueInfo(VulkanDeviceQueueType type) const {
    return queue_infos.at(type);
  }
  inline VulkanSwapchainSupportInfo GetSwapchainSupportInfo() const {
    return swapchain_support_info;
  }
  inline VkFormat GetDepthFormat() const { return depth_format; }
  inline size_t GetMinUBOAlignment() const {
    return properties.limits.minUniformBufferOffsetAlignment;
  }

  bool SupportsDeviceLocalHostVisible() const;
  /* TODO: this is unused rn. Possible uses includes getting rid of staging
   * copy and copying via memcpy */
  bool DeviceIsIntegrated() const;

private:
  bool SelectPhysicalDevice(VulkanPhysicalDeviceRequirements *requirements);
  bool DeviceExtensionsAvailable(VkPhysicalDevice physical_device,
                                 int required_extension_count,
                                 const char **required_extensions);

  VkPhysicalDevice physical_device;
  VkDevice logical_device;

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memory;

  std::unordered_map<VulkanDeviceQueueType, VulkanDeviceQueueInfo> queue_infos;
  VulkanSwapchainSupportInfo swapchain_support_info;
  VkFormat depth_format;
};