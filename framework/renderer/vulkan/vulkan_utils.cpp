#include "vulkan_utils.h"

#include "../../logger.h"
#include "vulkan_backend.h"

int32_t VulkanUtils::FindMemoryIndex(VkPhysicalDevice physical_device,
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

size_t VulkanUtils::GetDynamicAlignment(size_t element_size) {
  VulkanContext *context = VulkanBackend::GetContext();

  size_t min_ubo_alignment =
      context->device->GetProperties().limits.minUniformBufferOffsetAlignment;
  size_t dynamic_alignment = element_size;
  if (min_ubo_alignment > 0) {
    dynamic_alignment =
        (dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
  }

  return dynamic_alignment;
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
    return VK_FORMAT_R8G8B8A8_UNORM; /* TODO: what? */
  } break;
  case GPU_FORMAT_RGBA8: {
    /* TODO: this is device specific */
    return VK_FORMAT_R8G8B8A8_SRGB;
  } break;
  case GPU_FORMAT_D24_S8: {
    /* TODO: on my device, d24_s8 is not supported :( fix possibility to use
     * stencil buffer later */
    return context->device->GetDepthFormat();
  } break;
  case GPU_FORMAT_DEVICE_COLOR_OPTIMAL: {
    return context->swapchain->GetImageFormat().format;
  } break;
  case GPU_FORMAT_DEVICE_DEPTH_OPTIMAL: {
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
  case GPU_SHADER_STAGE_TYPE_GEOMETRY: {
    return VK_SHADER_STAGE_GEOMETRY_BIT;
  } break;
  case GPU_SHADER_STAGE_TYPE_TESSELATION_CONTROL: {
    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
  } break;
  case GPU_SHADER_STAGE_TYPE_TESSELATION_EVALUATION: {
    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
  } break;
  default: {
    ERROR("Unsupported shader stage!")
    return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
  } break;
  }

  return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}

VkImageAspectFlags
VulkanUtils::GPUTextureUsageToVulkanAspectFlags(GPUAttachmentUsage usage) {
  switch (usage) {
  case GPU_ATTACHMENT_USAGE_COLOR_ATTACHMENT: {
    return VK_IMAGE_ASPECT_COLOR_BIT;
  } break;
  case GPU_ATTACHMENT_USAGE_DEPTH_ATTACHMENT: {
    return VK_IMAGE_ASPECT_DEPTH_BIT;
  } break;
  case GPU_ATTACHMENT_USAGE_DEPTH_STENCIL_ATTACHMENT: {
    return VK_IMAGE_ASPECT_DEPTH_BIT; /* TODO: | VK_IMAGE_ASPECT_STENCIL_BIT */
  } break;
  case GPU_ATTACHMENT_USAGE_NONE: {
    return VK_IMAGE_ASPECT_NONE_KHR;
  } break;
  default: {
    ERROR("Unsupported texture usage!")
    return VK_IMAGE_ASPECT_NONE_KHR;
  };
  }

  return VK_IMAGE_ASPECT_NONE_KHR;
}

VkShaderStageFlags
VulkanUtils::GPUShaderStageFlagsToVulkanShaderStageFlags(uint8_t stage_flags) {
  VkShaderStageFlags result = VK_SHADER_STAGE_ALL_GRAPHICS; /* TODO: */
  if (stage_flags & GPU_SHADER_STAGE_TYPE_VERTEX) {
    result |= VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  if (stage_flags & GPU_SHADER_STAGE_TYPE_FRAGMENT) {
    result |= VK_SHADER_STAGE_VERTEX_BIT;
  }
  if (stage_flags & GPU_SHADER_STAGE_TYPE_GEOMETRY) {
    result |= VK_SHADER_STAGE_GEOMETRY_BIT;
  }
  if (stage_flags & GPU_SHADER_STAGE_TYPE_TESSELATION_CONTROL) {
    result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
  }
  if (stage_flags & GPU_SHADER_STAGE_TYPE_TESSELATION_EVALUATION) {
    result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
  }

  return result;
}

VkPrimitiveTopology
VulkanUtils::GPUShaderTopologyTypeToVulkanTopology(GPUShaderTopologyType type) {
  switch (type) {
  case GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST: {
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  } break;
  case GPU_SHADER_TOPOLOGY_TYPE_LINE_LIST: {
    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  } break;
  default: {
    ERROR("Unsupported topology type!");
    return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
  } break;
  }

  return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}