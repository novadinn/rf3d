#include "vulkan_utils.h"

#include "../../logger.h"
#include "vulkan_backend.h"

int VulkanUtils::FindMemoryIndex(VkPhysicalDevice physical_device,
                                 uint32_t type_filter,
                                 VkMemoryPropertyFlags property_flags) {
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
    if (type_filter & (1 << i) &&
        (memory_properties.memoryTypes[i].propertyFlags & property_flags) ==
            property_flags) {
      return i;
    }
  }

  ERROR("Failed to find memory index");

  return -1;
}

VkFormat VulkanUtils::GPUFormatToVulkanFormat(GPUFormat format) {
  VulkanContext *context = VulkanBackend::GetContext();

  switch (format) {
  case GPU_FORMAT_RG32F: {
    return VK_FORMAT_R32G32_SFLOAT;
  } break;
  case GPU_FORMAT_RGB32F: {
    return VK_FORMAT_R32G32B32_SFLOAT;
  } break;
  case GPU_FORMAT_RGB8: {
    return VK_FORMAT_R8G8B8_UNORM;
  } break;
  case GPU_FORMAT_RGBA8: {
    /* TODO: this is device specific */
    return VK_FORMAT_B8G8R8A8_UNORM;
  } break;
  case GPU_FORMAT_D24_S8: {
    /* TODO: on my device, d24_s8 is not supported :( fix possibility to use
     * stencil buffer later */
    return context->device->GetDepthFormat();
  } break;
  default: {
    ERROR("Unsupported shader attribute format!");
    return VK_FORMAT_UNDEFINED;
  } break;
  }

  return VK_FORMAT_UNDEFINED;
}

VkShaderStageFlagBits
VulkanUtils::GPUShaderStageTypeToVulkanStage(GPUShaderStageType stage) {
  switch (stage) {
  case GPU_SHADER_STAGE_TYPE_VERTEX: {
    return VK_SHADER_STAGE_VERTEX_BIT;
  } break;
  case GPU_SHADER_STAGE_TYPE_FRAGMENT: {
    return VK_SHADER_STAGE_FRAGMENT_BIT;
  } break;
  default: {
    ERROR("Unsupported shader stage!")
    return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
  } break;
  }

  return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}

VkImageAspectFlags
VulkanUtils::GPUTextureUsageToVulkanAspectFlags(GPUTextureUsage usage) {
  switch (usage) {
  case GPU_TEXTURE_USAGE_COLOR_ATTACHMENT: {
    return VK_IMAGE_ASPECT_COLOR_BIT;
  } break;
  case GPU_TEXTURE_USAGE_DEPTH_ATTACHMENT: {
    return VK_IMAGE_ASPECT_DEPTH_BIT;
  } break;
  case GPU_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT: {
    return VK_IMAGE_ASPECT_DEPTH_BIT; /* TODO: | VK_IMAGE_ASPECT_STENCIL_BIT */
  } break;
  case GPU_TEXTURE_USAGE_NONE: {
    return VK_IMAGE_ASPECT_NONE_KHR;
  } break;
  default: {
    ERROR("Unsupported texture usage!")
    return VK_IMAGE_ASPECT_NONE_KHR;
  };
  }

  return VK_IMAGE_ASPECT_NONE_KHR;
}

VkDescriptorType VulkanUtils::GPUShaderBufferTypeToVulkanDescriptorType(
    GPUShaderBufferType type) {
  switch (type) {
  case GPU_SHADER_BUFFER_TYPE_UNIFORM_BUFFER: {
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  } break;
  default: {
    ERROR("Unsupported shader descriptor type!")
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
  } break;
  }

  return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

VkShaderStageFlags
VulkanUtils::GPUShaderStageFlagsToVulkanShaderStageFlags(uint8_t stage_flags) {
  /* TODO: */

  return VK_SHADER_STAGE_ALL_GRAPHICS;
}