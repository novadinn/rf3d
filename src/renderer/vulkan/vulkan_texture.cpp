#include "vulkan_texture.h"

#include "logger.h"
#include "renderer/gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

void VulkanTexture::Create(GPUFormat image_format,
                           GPUTextureAspect texture_aspect,
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

  image.Create(width, height, native_format, VK_IMAGE_TILING_OPTIMAL, usage,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, native_aspect_flags);
  image.CreateImageView(native_format, native_aspect_flags);

  VkSamplerCreateInfo sampler_create_info = {};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.pNext = 0;
  sampler_create_info.flags = 0;
  sampler_create_info.magFilter = VK_FILTER_LINEAR;
  sampler_create_info.minFilter = VK_FILTER_LINEAR;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_create_info.mipLodBias = 0.0f;
  sampler_create_info.anisotropyEnable = VK_FALSE;
  sampler_create_info.maxAnisotropy = 16;
  sampler_create_info.compareEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_create_info.minLod = 0.0f;
  sampler_create_info.maxLod = 0.0f;
  sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_create_info.unnormalizedCoordinates = VK_FALSE;

  VK_CHECK(vkCreateSampler(context->device->GetLogicalDevice(),
                           &sampler_create_info, context->allocator, &sampler));
}

void VulkanTexture::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  vkDestroySampler(context->device->GetLogicalDevice(), sampler,
                   context->allocator);
  image.Destroy();
}

void VulkanTexture::WriteData(uint8_t *pixels, uint32_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  uint32_t channel_count = GPUUtils::GetGPUFormatCount(format);
  uint32_t size = image.GetWidth() * image.GetHeight() * channel_count;

  VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkMemoryPropertyFlags memory_prop_flags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  VulkanBuffer staging;
  staging.Create(GPU_BUFFER_TYPE_STAGING, size);
  staging.LoadData(0, size, pixels);

  VulkanDeviceQueueInfo queue_info;
  context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS, &queue_info);
  VkCommandPool command_pool = queue_info.command_pool;
  VkQueue queue = queue_info.queue;

  VulkanCommandBuffer temp_command_buffer;
  temp_command_buffer.AllocateAndBeginSingleUse(command_pool);

  image.TransitionLayout(&temp_command_buffer, native_format,
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  image.CopyFromBuffer(&staging, &temp_command_buffer);
  image.TransitionLayout(&temp_command_buffer, native_format,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  temp_command_buffer.FreeAndEndSingleUse(command_pool, queue);
  staging.Destroy();
}

void VulkanTexture::Resize(uint32_t new_width, uint32_t new_height) {
  image.Destroy();
  image.Create(width, height, native_format, VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, native_aspect_flags);
  image.CreateImageView(native_format, native_aspect_flags);
}