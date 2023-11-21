#pragma once

#include "../gpu_shader.h"
#include "../gpu_texture.h"

#include <vulkan/vulkan.h>

class VulkanUtils {
public:
  static int32_t FindMemoryIndex(VkPhysicalDevice physical_device,
                                 uint32_t type_filter,
                                 VkMemoryPropertyFlags property_flags);
  static size_t GetDynamicAlignment(size_t element_size);
  static VkFormat GPUFormatToVulkanFormat(GPUFormat format);
  static VkShaderStageFlagBits
  GPUShaderStageTypeToVulkanStage(GPUShaderStageType stage);
  static VkImageAspectFlags
  GPUTextureUsageToVulkanAspectFlags(GPUAttachmentUsage usage);
  static VkShaderStageFlags
  GPUShaderStageFlagsToVulkanShaderStageFlags(uint8_t stage_flags);
  static VkPrimitiveTopology
  GPUShaderTopologyTypeToVulkanTopology(GPUShaderTopologyType type);
};