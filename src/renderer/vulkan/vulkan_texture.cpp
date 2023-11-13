#include "vulkan_texture.h"

#include "logger.h"
#include "renderer/gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

void VulkanTexture::Create(GPUFormat texture_format,
                           GPUTextureType texture_type, uint32_t texture_width,
                           uint32_t texture_height) {
  VulkanContext *context = VulkanBackend::GetContext();

  format = texture_format;
  type = texture_type;
  width = texture_width;
  height = texture_height;

  VkFormat native_format = VulkanUtils::GPUFormatToVulkanFormat(format);
  VkImageAspectFlags native_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

  VkImageType image_type = VK_IMAGE_TYPE_MAX_ENUM;
  uint32_t array_layers = 0;
  VkImageCreateFlags create_flags = 0;
  switch (type) {
  case GPU_TEXTURE_TYPE_2D: {
    image_type = VK_IMAGE_TYPE_2D;
    array_layers = 1;
  } break;
  default: {
    ERROR("Unsupported image type!");
  } break;
  }

  /* TODO: a lot of hardcoded stuff */
  VkImageCreateInfo image_create_info = {};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.pNext = 0;
  image_create_info.flags = create_flags;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = native_format;
  image_create_info.extent.width = width;
  image_create_info.extent.height = height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = array_layers;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.queueFamilyIndexCount = 0;
  image_create_info.pQueueFamilyIndices = 0;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VK_CHECK(vkCreateImage(context->device->GetLogicalDevice(),
                         &image_create_info, context->allocator, &handle));

  VkMemoryRequirements memory_requirements = {};
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

  VkSamplerCreateInfo sampler_create_info = {};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.pNext = 0;
  sampler_create_info.flags = 0;
  sampler_create_info.magFilter = VK_FILTER_LINEAR;
  sampler_create_info.minFilter = VK_FILTER_LINEAR;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.mipLodBias = 0.0f;
  /* TODO: */
  // if (context->device->GetFeatures().samplerAnisotropy) {
  //   sampler_create_info.anisotropyEnable = VK_TRUE;
  //   sampler_create_info.maxAnisotropy =
  //       context->device->GetProperties().limits.maxSamplerAnisotropy;
  // } else {
  //   sampler_create_info.anisotropyEnable = VK_FALSE;
  //   sampler_create_info.maxAnisotropy = 1.0f;
  // }
  sampler_create_info.anisotropyEnable = VK_FALSE;
  sampler_create_info.maxAnisotropy = 1.0f;
  sampler_create_info.compareEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_create_info.minLod = 0.0f;
  sampler_create_info.maxLod = 0.0f;
  sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  sampler_create_info.unnormalizedCoordinates = VK_FALSE;

  VK_CHECK(vkCreateSampler(context->device->GetLogicalDevice(),
                           &sampler_create_info, context->allocator, &sampler));
}

void VulkanTexture::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  vkDestroySampler(context->device->GetLogicalDevice(), sampler,
                   context->allocator);

  vkDestroyImageView(context->device->GetLogicalDevice(), view,
                     context->allocator);
  vkFreeMemory(context->device->GetLogicalDevice(), memory, context->allocator);
  vkDestroyImage(context->device->GetLogicalDevice(), handle,
                 context->allocator);

  format = GPU_FORMAT_NONE;
  type = GPU_TEXTURE_TYPE_NONE;
  handle = 0;
  view = 0;
  memory = 0;
}

void VulkanTexture::WriteData(void *pixels, uint32_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  VkFormat native_format = VulkanUtils::GPUFormatToVulkanFormat(format);

  uint32_t channel_count = GPUUtils::GetGPUFormatCount(format);
  uint32_t size = width * height * channel_count;

  VulkanBuffer staging;
  staging.Create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  staging.LoadData(0, size, pixels);

  /* TODO: or transfer? */
  VulkanDeviceQueueInfo queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);
  VkCommandPool command_pool = queue_info.command_pool;
  VkQueue queue = queue_info.queue;

  VulkanCommandBuffer temp_command_buffer;
  temp_command_buffer.AllocateAndBeginSingleUse(command_pool);

  TransitionLayout(&temp_command_buffer, native_format,
                   VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  CopyFromBuffer(&staging, &temp_command_buffer, 0);
  TransitionLayout(&temp_command_buffer, native_format,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  temp_command_buffer.FreeAndEndSingleUse(command_pool, queue);
  staging.Destroy();
}

void VulkanTexture::TransitionLayout(VulkanCommandBuffer *command_buffer,
                                     VkFormat format, VkImageLayout old_layout,
                                     VkImageLayout new_layout) {
  VulkanContext *context = VulkanBackend::GetContext();

  /* TODO: or transfer? */
  VulkanDeviceQueueInfo graphics_queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  uint32_t array_layers = 0;
  switch (type) {
  case GPU_TEXTURE_TYPE_2D: {
    array_layers = 1;
  } break;
  default: {
    ERROR("Unsupported image type!");
  } break;
  }

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
  barrier.subresourceRange.layerCount = array_layers;

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
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else {
    FATAL("Unsupported layout transition!");
    return;
  }

  vkCmdPipelineBarrier(command_buffer->GetHandle(), source_stage, dest_stage, 0,
                       0, 0, 0, 0, 1, &barrier);
}

void VulkanTexture::CopyFromBuffer(VulkanBuffer *buffer,
                                   VulkanCommandBuffer *command_buffer,
                                   uint64_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  uint32_t array_layers = 0;
  switch (type) {
  case GPU_TEXTURE_TYPE_2D: {
    array_layers = 1;
  } break;
  default: {
    ERROR("Unsupported image type!");
  } break;
  }

  VkBufferImageCopy region = {};
  region.bufferOffset = offset;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = array_layers;
  region.imageOffset.x = 0;
  region.imageOffset.y = 0;
  region.imageOffset.z = 0;
  region.imageExtent.width = width;
  region.imageExtent.height = height;
  region.imageExtent.depth = 1;

  VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(),
                              buffer->GetHandle(), buffer->GetMemory(), 0));
  vkCmdCopyBufferToImage(command_buffer->GetHandle(), buffer->GetHandle(),
                         handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &region);
}