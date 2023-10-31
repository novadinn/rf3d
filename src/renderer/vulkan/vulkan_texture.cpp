#include "vulkan_texture.h"

#include "logger.h"
#include "renderer/gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

void VulkanTexture::Create(GPUFormat image_format,
                           GPUTextureUsage texture_aspect,
                           uint32_t texture_width, uint32_t texture_height) {
  VulkanContext *context = VulkanBackend::GetContext();

  format = image_format;
  aspect = texture_aspect;
  width = texture_width;
  height = texture_height;
  native_format = VulkanUtils::GPUFormatToVulkanFormat(format);
  native_aspect_flags = VulkanUtils::GPUTextureUsageToVulkanAspectFlags(aspect);

  VkImageUsageFlags usage;
  if (GPUUtils::IsDepthFormat(format)) {
    usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  } else {
    usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }

  VkMemoryRequirements memory_requirements;

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

void VulkanTexture::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  vkDestroyImageView(context->device->GetLogicalDevice(), view,
                     context->allocator);
  vkFreeMemory(context->device->GetLogicalDevice(), memory, context->allocator);
  vkDestroyImage(context->device->GetLogicalDevice(), handle,
                 context->allocator);

  handle = 0;
  view = 0;
  memory = 0;
  native_format = VK_FORMAT_UNDEFINED;
  native_aspect_flags = VK_IMAGE_ASPECT_NONE;
}

void VulkanTexture::WriteData(uint8_t *pixels, uint32_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  uint32_t channel_count = GPUUtils::GetGPUFormatCount(format);
  uint32_t size = width * height * channel_count;

  VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkMemoryPropertyFlags memory_prop_flags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  VulkanBuffer staging;
  staging.Create(GPU_BUFFER_TYPE_STAGING, size);
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
  CopyFromBuffer(&staging, &temp_command_buffer);
  TransitionLayout(&temp_command_buffer, native_format,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  temp_command_buffer.FreeAndEndSingleUse(command_pool, queue);
  staging.Destroy();
}

void VulkanTexture::Resize(uint32_t new_width, uint32_t new_height) {
  Destroy();
  Create(format, aspect, new_width, new_height);
}

void VulkanTexture::TransitionLayout(VulkanCommandBuffer *command_buffer,
                                     VkFormat format, VkImageLayout old_layout,
                                     VkImageLayout new_layout) {}

void VulkanTexture::CopyFromBuffer(VulkanBuffer *buffer,
                                   VulkanCommandBuffer *command_buffer) {}