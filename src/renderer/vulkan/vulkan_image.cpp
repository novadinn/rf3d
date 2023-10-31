#include "vulkan_image.h"

#include "../../logger.h"

#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_device.h"
#include "vulkan_utils.h"

void VulkanImage::Create(int image_width, int image_height, VkFormat format,
                         VkImageTiling tiling, VkImageUsageFlags usage_flags,
                         VkMemoryPropertyFlags memory_flags,
                         VkImageAspectFlags aspect_flags) {
  VulkanContext *context = VulkanBackend::GetContext();

  VkMemoryRequirements memory_requirements;
  width = image_width;
  height = image_height;

  /* TODO: a lot of hardcoded stuff */
  VkImageCreateInfo image_create_info = {};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.pNext = 0;
  image_create_info.flags = 0;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = format;
  image_create_info.extent.width = width;
  image_create_info.extent.height = height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 4;
  image_create_info.arrayLayers = 1;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.tiling = tiling;
  image_create_info.usage = usage_flags;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.queueFamilyIndexCount = 0;
  image_create_info.pQueueFamilyIndices = 0;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VK_CHECK(vkCreateImage(context->device->GetLogicalDevice(),
                         &image_create_info, context->allocator, &handle));

  vkGetImageMemoryRequirements(context->device->GetLogicalDevice(), handle,
                               &memory_requirements);

  /* find memory index */
  int memory_type = VulkanUtils::FindMemoryIndex(
      context->device->GetPhysicalDevice(), memory_requirements.memoryTypeBits,
      memory_flags);
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
}

void VulkanImage::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDestroyImageView(context->device->GetLogicalDevice(), view,
                     context->allocator);
  vkFreeMemory(context->device->GetLogicalDevice(), memory, context->allocator);
  vkDestroyImage(context->device->GetLogicalDevice(), handle,
                 context->allocator);

  handle = 0;
  view = 0;
  memory = 0;
  width = 0;
  height = 0;
}

void VulkanImage::CreateImageView(VkFormat format,
                                  VkImageAspectFlags aspect_flags) {
  VulkanContext *context = VulkanBackend::GetContext();

  VkImageViewCreateInfo view_create_info = {};
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = 0;
  view_create_info.flags = 0;
  view_create_info.image = handle;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = format;
  /* view_create_info.components; */
  view_create_info.subresourceRange.aspectMask = aspect_flags;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;

  VK_CHECK(vkCreateImageView(context->device->GetLogicalDevice(),
                             &view_create_info, context->allocator, &view));
}

void VulkanImage::TransitionLayout(VulkanCommandBuffer *command_buffer,
                                   VkFormat format, VkImageLayout old_layout,
                                   VkImageLayout new_layout) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo graphics_queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = graphics_queue_info.family_index;
  barrier.dstQueueFamilyIndex = graphics_queue_info.family_index;
  barrier.image = handle;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags dest_stage;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    FATAL("Unsupported layout transition!");
    return;
  }

  vkCmdPipelineBarrier(command_buffer->GetHandle(), source_stage, dest_stage, 0,
                       0, 0, 0, 0, 1, &barrier);
}

void VulkanImage::CopyFromBuffer(VulkanBuffer *buffer,
                                 VulkanCommandBuffer *command_buffer) {
  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageExtent.width = width;
  region.imageExtent.height = height;
  region.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(command_buffer->GetHandle(), buffer->GetHandle(),
                         handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &region);
}