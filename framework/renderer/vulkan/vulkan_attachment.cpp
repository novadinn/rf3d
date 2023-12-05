#include "vulkan_attachment.h"

#include "../../logger.h"
#include "../gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_debug_marker.h"
#include "vulkan_utils.h"

void VulkanAttachment::Create(GPUFormat attachment_format,
                              GPUAttachmentUsage attachment_usage,
                              uint32_t attachment_width,
                              uint32_t attachment_height) {
  VulkanContext *context = VulkanBackend::GetContext();

  format = attachment_format;
  aspect = attachment_usage;
  width = attachment_width;
  height = attachment_height;
  VkFormat native_format = VulkanUtils::GPUFormatToVulkanFormat(format);
  VkImageAspectFlags native_aspect_flags =
      VulkanUtils::GPUTextureUsageToVulkanAspectFlags(aspect);

  VkImageUsageFlags usage;
  /* TODO: all of the attachments are sampled for now */
  if (GPUUtils::IsDepthFormat(format)) {
    usage = VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  } else {
    usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }

  VkImageCreateInfo image_create_info = {};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.pNext = 0;
  image_create_info.flags = 0;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = native_format;
  image_create_info.extent.width = width;
  image_create_info.extent.height = height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage = usage;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.queueFamilyIndexCount = 0;
  image_create_info.pQueueFamilyIndices = 0;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VmaAllocationCreateInfo vma_allocation_create_info = {};
  /* vma_allocation_create_info.flags; */
  vma_allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  vma_allocation_create_info.requiredFlags =
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  /* vma_allocation_create_info.preferredFlags;
  vma_allocation_create_info.memoryTypeBits;
  vma_allocation_create_info.pool;
  vma_allocation_create_info.pUserData;
  vma_allocation_create_info.priority; */

  VK_CHECK(vmaCreateImage(context->vma_allocator, &image_create_info,
                          &vma_allocation_create_info, &handle, &memory, 0));

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

  /* TODO: create sampler only if needed, and only if it is a color attachment
   */
  VkSamplerCreateInfo sampler_create_info = {};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.pNext = 0;
  sampler_create_info.flags = 0;
  /* TODO: linear may not be filterable */
  sampler_create_info.magFilter = VK_FILTER_LINEAR;
  sampler_create_info.minFilter = VK_FILTER_LINEAR;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.mipLodBias = 0.0f;
  sampler_create_info.maxAnisotropy = 1.0f;
  sampler_create_info.minLod = 0.0f;
  sampler_create_info.maxLod = 1.0f;
  sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK(vkCreateSampler(context->device->GetLogicalDevice(),
                           &sampler_create_info, context->allocator, &sampler));
}

void VulkanAttachment::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  vkDestroySampler(context->device->GetLogicalDevice(), sampler,
                   context->allocator);

  vkDestroyImageView(context->device->GetLogicalDevice(), view,
                     context->allocator);
  vmaDestroyImage(context->vma_allocator, handle, memory);

  format = GPU_FORMAT_NONE;
  aspect = GPU_ATTACHMENT_USAGE_NONE;
  handle = 0;
  view = 0;
  memory = 0;
}

void VulkanAttachment::SetDebugName(const char *name) {
  VulkanDebugUtils::SetObjectName(name, (uint64_t)handle, VK_OBJECT_TYPE_IMAGE);
  /* TODO: set sampler and view names. other structures that have those
   * functions should set their data accordingly */
}

void VulkanAttachment::SetDebugTag(const void *tag, size_t tag_size) {
  VulkanDebugUtils::SetObjectTag(tag, (uint64_t)handle, VK_OBJECT_TYPE_IMAGE, 0,
                                 tag_size);
}

void VulkanAttachment::CreateAsSwapchainAttachment(VkImage new_handle,
                                                   VkImageView new_view) {
  handle = new_handle;
  view = new_view;
}

void VulkanAttachment::DestroyAsSwapchainAttachment() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDestroyImageView(context->device->GetLogicalDevice(), view,
                     context->allocator);
}