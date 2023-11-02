#include "vulkan_attachment.h"

#include "logger.h"
#include "renderer/gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_utils.h"

void VulkanAttachment::Create(GPUFormat attachment_format,
                              GPUAttachmentUsage attachment_usage,
                              uint32_t texture_width, uint32_t texture_height) {
  VulkanContext *context = VulkanBackend::GetContext();

  format = attachment_format;
  aspect = attachment_usage;
  width = texture_width;
  height = texture_height;
  VkFormat native_format = VulkanUtils::GPUFormatToVulkanFormat(format);
  VkImageAspectFlags native_aspect_flags =
      VulkanUtils::GPUTextureUsageToVulkanAspectFlags(aspect);

  VkImageUsageFlags usage;
  if (GPUUtils::IsDepthFormat(format)) {
    usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  } else {
    usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }

  /* TODO: a lot of hardcoded stuff */
  VkImageCreateInfo image_create_info = {};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.pNext = 0;
  image_create_info.flags = 0;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = native_format;
  image_create_info.extent.width = width;
  image_create_info.extent.height = height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 4;
  image_create_info.arrayLayers = 1;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage = usage;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.queueFamilyIndexCount = 0;
  image_create_info.pQueueFamilyIndices = 0;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkMemoryRequirements memory_requirements = {};

  VK_CHECK(vkCreateImage(context->device->GetLogicalDevice(),
                         &image_create_info, context->allocator, &handle));

  vkGetImageMemoryRequirements(context->device->GetLogicalDevice(), handle,
                               &memory_requirements);

  /* find memory index */
  int memory_type = VulkanUtils::FindMemoryIndex(
      context->device->GetPhysicalDevice(), memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memory_type == -1) {
    ERROR("Required memory type not found. Image not valid.");
  }

  VkMemoryAllocateInfo memory_allocate_info = {};
  memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memory_allocate_info.pNext = 0;
  memory_allocate_info.allocationSize = memory_requirements.size;
  memory_allocate_info.memoryTypeIndex = memory_type;

  VK_CHECK(vkAllocateMemory(context->device->GetLogicalDevice(),
                            &memory_allocate_info, context->allocator,
                            &memory));

  VK_CHECK(vkBindImageMemory(context->device->GetLogicalDevice(), handle,
                             memory, 0));

  VkImageViewCreateInfo view_create_info = {};
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = 0;
  view_create_info.flags = 0;
  view_create_info.image = handle;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = native_format;
  /* view_create_info.components; */
  view_create_info.subresourceRange.aspectMask = native_aspect_flags;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;

  VK_CHECK(vkCreateImageView(context->device->GetLogicalDevice(),
                             &view_create_info, context->allocator, &view));
}

void VulkanAttachment::Destroy() {}