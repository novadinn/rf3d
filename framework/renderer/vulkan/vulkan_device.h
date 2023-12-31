#pragma once

#include "vulkan_command_buffer.h"

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

enum VulkanDeviceQueueType {
  VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS,
  VULKAN_DEVICE_QUEUE_TYPE_PRESENT,
  VULKAN_DEVICE_QUEUE_TYPE_TRANSFER,
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
  uint32_t family_index;
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

  bool SupportsDeviceLocalHostVisible() const;
  bool TransferQueueIsOnly() const;

  inline VkPhysicalDeviceProperties GetProperties() const { return properties; }
  inline VkPhysicalDeviceFeatures GetFeatures() const { return features; }
  inline VkPhysicalDeviceMemoryProperties GetMemoryProperties() const {
    return memory;
  }

private:
  bool SelectPhysicalDevice(VulkanPhysicalDeviceRequirements *requirements);
  bool DeviceExtensionsAvailable(VkPhysicalDevice physical_device,
                                 uint32_t required_extension_count,
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