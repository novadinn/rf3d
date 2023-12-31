#include "vulkan_texture.h"

#include "../../logger.h"
#include "../gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_debug_marker.h"
#include "vulkan_utils.h"

bool VulkanTexture::Create(GPUFormat texture_format,
                           GPUTextureType texture_type, uint32_t texture_width,
                           uint32_t texture_height) {
  VulkanContext *context = VulkanBackend::GetContext();

  uint32_t mip_levels =
      static_cast<uint32_t>(
          std::floor(std::log2(std::max(texture_width, texture_height)))) +
      1;
  /* TODO: mip levels for a cubemap are not supported right now */
  if (texture_type == GPU_TEXTURE_TYPE_CUBEMAP) {
    mip_levels = 1;
  }

  format = texture_format;
  type = texture_type;
  width = texture_width;
  height = texture_height;

  VkFormat native_format = VulkanUtils::GPUFormatToVulkanFormat(format);
  VkImageAspectFlags native_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

  VkImageType image_type = VK_IMAGE_TYPE_MAX_ENUM;
  uint32_t array_layers = 0;
  VkImageCreateFlags create_flags = 0;
  VkImageViewType view_type;
  switch (type) {
  case GPU_TEXTURE_TYPE_2D: {
    image_type = VK_IMAGE_TYPE_2D;
    array_layers = 1;
    view_type = VK_IMAGE_VIEW_TYPE_2D;
  } break;
  case GPU_TEXTURE_TYPE_CUBEMAP: {
    image_type = VK_IMAGE_TYPE_2D;
    array_layers = 6;
    create_flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    view_type = VK_IMAGE_VIEW_TYPE_CUBE;
  } break;
  default: {
    ERROR("Unsupported image type!");
    return false;
  } break;
  }

  VkImageCreateInfo image_create_info = {};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.pNext = 0;
  image_create_info.flags = create_flags;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = native_format;
  image_create_info.extent.width = width;
  image_create_info.extent.height = height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = mip_levels;
  image_create_info.arrayLayers = array_layers;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT;
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
  view_create_info.viewType = view_type;
  view_create_info.format = native_format;
  /* view_create_info.components; */
  view_create_info.subresourceRange.aspectMask = native_aspect_flags;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = mip_levels;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = array_layers;

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
  if (context->device->GetFeatures().samplerAnisotropy) {
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy =
        context->device->GetProperties().limits.maxSamplerAnisotropy;
  } else {
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.maxAnisotropy = 1.0f;
  }
  sampler_create_info.compareEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
  sampler_create_info.minLod = 0.0f;
  sampler_create_info.maxLod = (float)mip_levels;
  sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  sampler_create_info.unnormalizedCoordinates = VK_FALSE;

  VK_CHECK(vkCreateSampler(context->device->GetLogicalDevice(),
                           &sampler_create_info, context->allocator, &sampler));

  return true;
}

void VulkanTexture::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  vkDestroySampler(context->device->GetLogicalDevice(), sampler,
                   context->allocator);

  vkDestroyImageView(context->device->GetLogicalDevice(), view,
                     context->allocator);
  vmaDestroyImage(context->vma_allocator, handle, memory);

  format = GPU_FORMAT_NONE;
  type = GPU_TEXTURE_TYPE_NONE;
  handle = 0;
  view = 0;
  memory = 0;
}

void VulkanTexture::WriteData(void *pixels, uint32_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  uint32_t mip_levels =
      static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
  /* TODO: mip levels for a cubemap are not supported right now */
  if (type == GPU_TEXTURE_TYPE_CUBEMAP) {
    mip_levels = 1;
  }

  uint32_t array_layers = 0;
  switch (type) {
  case GPU_TEXTURE_TYPE_2D: {
    array_layers = 1;
  } break;
  case GPU_TEXTURE_TYPE_CUBEMAP: {
    array_layers = 6;
  } break;
  default: {
    ERROR("Unsupported image type!");
    array_layers = 1;
  } break;
  }

  VkFormat native_format = VulkanUtils::GPUFormatToVulkanFormat(format);

  uint32_t channel_count = GPUUtils::GetGPUFormatCount(format);
  uint32_t size = width * height * channel_count * array_layers;

  VulkanBuffer staging;
  staging.Create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 VMA_MEMORY_USAGE_CPU_ONLY);
  staging.LoadData(0, size, pixels);

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
  if (mip_levels < 2) {
    TransitionLayout(&temp_command_buffer, native_format,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  temp_command_buffer.FreeAndEndSingleUse(command_pool, queue);
  staging.Destroy();

  if (!(mip_levels < 2)) {
    if (!GenerateMipMaps()) {
      TransitionLayout(&temp_command_buffer, native_format,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
  }
}

void VulkanTexture::SetDebugName(const char *name) {
  VulkanDebugUtils::SetObjectName(name, (uint64_t)handle, VK_OBJECT_TYPE_IMAGE);
}

void VulkanTexture::SetDebugTag(const void *tag, size_t tag_size) {
  VulkanDebugUtils::SetObjectTag(tag, (uint64_t)handle, VK_OBJECT_TYPE_IMAGE, 0,
                                 tag_size);
}

void VulkanTexture::TransitionLayout(VulkanCommandBuffer *command_buffer,
                                     VkFormat format, VkImageLayout old_layout,
                                     VkImageLayout new_layout) {
  VulkanContext *context = VulkanBackend::GetContext();

  uint32_t mip_levels =
      static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
  if (type == GPU_TEXTURE_TYPE_CUBEMAP) {
    mip_levels = 1;
  }

  VulkanDeviceQueueInfo graphics_queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  uint32_t array_layers = 0;
  switch (type) {
  case GPU_TEXTURE_TYPE_2D: {
    array_layers = 1;
  } break;
  case GPU_TEXTURE_TYPE_CUBEMAP: {
    array_layers = 6;
  } break;
  default: {
    ERROR("Unsupported image type!");
    array_layers = 1;
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
  barrier.subresourceRange.levelCount = mip_levels;
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
    ERROR("Unsupported layout transition!");
    return;
  }

  vkCmdPipelineBarrier(command_buffer->GetHandle(), source_stage, dest_stage, 0,
                       0, 0, 0, 0, 1, &barrier);
}

void VulkanTexture::CopyFromBuffer(VulkanBuffer *buffer,
                                   VulkanCommandBuffer *command_buffer,
                                   uint64_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  uint32_t mip_levels =
      static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
  if (type == GPU_TEXTURE_TYPE_CUBEMAP) {
    mip_levels = 1;
  }

  uint32_t array_layers = 0;
  switch (type) {
  case GPU_TEXTURE_TYPE_2D: {
    array_layers = 1;
  } break;
  case GPU_TEXTURE_TYPE_CUBEMAP: {
    array_layers = 6;
  } break;
  default: {
    ERROR("Unsupported image type!");
    array_layers = 1;
  } break;
  }

  uint32_t channel_count = GPUUtils::GetGPUFormatCount(format);
  uint32_t side_size = width * height * channel_count;

  std::vector<VkBufferImageCopy> image_copies;
  for (uint32_t i = 0; i < array_layers; ++i) {
    for (uint32_t j = 0; j < mip_levels; ++j) {
      VkBufferImageCopy region = {};
      region.bufferOffset = side_size * i;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = j;
      region.imageSubresource.baseArrayLayer = i;
      region.imageSubresource.layerCount = 1;
      region.imageOffset.x = 0;
      region.imageOffset.y = 0;
      region.imageOffset.z = 0;
      region.imageExtent.width = std::max(1u, width >> j);
      region.imageExtent.height = std::max(1u, height >> j);
      region.imageExtent.depth = 1;

      image_copies.emplace_back(region);
    }
  }

  // VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(),
  //                             buffer->GetHandle(), buffer->GetMemory(), 0));
  // vkCmdCopyBufferToImage(command_buffer->GetHandle(), buffer->GetHandle(),
  //                        handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
  //                        image_copies.size(), image_copies.data());
  vkCmdCopyBufferToImage(command_buffer->GetHandle(), buffer->GetHandle(),
                         handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         image_copies.size(), image_copies.data());
}

bool VulkanTexture::GenerateMipMaps() {
  VulkanContext *context = VulkanBackend::GetContext();

  uint32_t mip_levels =
      static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
  if (type == GPU_TEXTURE_TYPE_CUBEMAP) {
    mip_levels = 1;
  }

  VkFormat native_format = VulkanUtils::GPUFormatToVulkanFormat(format);

  VkFormatProperties format_properties;
  vkGetPhysicalDeviceFormatProperties(context->device->GetPhysicalDevice(),
                                      native_format, &format_properties);

  if (!(format_properties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    ERROR("Texture image format does not support linear blitting!");
    return false;
  }

  VulkanDeviceQueueInfo queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);
  VkCommandPool command_pool = queue_info.command_pool;
  VkQueue queue = queue_info.queue;

  VulkanCommandBuffer temp_command_buffer;
  temp_command_buffer.AllocateAndBeginSingleUse(command_pool);

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = handle;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mip_width = width;
  int32_t mip_height = height;

  for (uint32_t i = 1; i < mip_levels; ++i) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(
        temp_command_buffer.GetHandle(), VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkImageBlit blit = {};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mip_width, mip_height, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1,
                          mip_height > 1 ? mip_height / 2 : 1, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(temp_command_buffer.GetHandle(), handle,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, handle,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(temp_command_buffer.GetHandle(),
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

    if (mip_width > 1)
      mip_width /= 2;
    if (mip_height > 1)
      mip_height /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mip_levels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(temp_command_buffer.GetHandle(),
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  temp_command_buffer.FreeAndEndSingleUse(command_pool, queue);

  return true;
}