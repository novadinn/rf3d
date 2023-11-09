#pragma once

#include "renderer/gpu_shader.h"
#include "renderer/gpu_texture.h"

#include <vulkan/vulkan.h>

class VulkanUtils {
public:
  static int FindMemoryIndex(VkPhysicalDevice physical_device,
                             uint32_t type_filter,
                             VkMemoryPropertyFlags property_flags);
  static size_t GetDynamicAlignment(size_t element_size);
  static VkFormat GPUFormatToVulkanFormat(GPUFormat format);
  static VkShaderStageFlagBits
  GPUShaderStageTypeToVulkanStage(GPUShaderStageType stage);
  static VkImageAspectFlags
  GPUTextureUsageToVulkanAspectFlags(GPUAttachmentUsage usage);
  /* TODO: not type, flags */
  static VkShaderStageFlags
  GPUShaderStageFlagsToVulkanShaderStageFlags(uint8_t stage_flags);
};