#include "vulkan_descriptor_set.h"

#include "../../logger.h"
#include "vulkan_backend.h"
#include "vulkan_debug_marker.h"
#include "vulkan_descriptor_builder.h"
#include "vulkan_texture.h"
#include "vulkan_uniform_buffer.h"

void VulkanDescriptorSet::Create(
    std::vector<GPUDescriptorBinding> &set_bindings) {
  VulkanContext *context = VulkanBackend::GetContext();

  bindings = set_bindings;

  VulkanDescriptorBuilder builder = VulkanDescriptorBuilder::Begin();

  for (uint32_t i = 0; i < bindings.size(); ++i) {
    GPUDescriptorBinding &binding = bindings[i];
    switch (binding.type) {
    case GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER: {
      VulkanUniformBuffer *native_uniform_buffer =
          (VulkanUniformBuffer *)binding.uniform_buffer;

      VkDescriptorBufferInfo buffer_info = {};
      buffer_info.buffer = native_uniform_buffer->GetBuffer().GetHandle();
      buffer_info.offset = 0;
      buffer_info.range = native_uniform_buffer->GetDynamicAlignment();

      // VK_CHECK(vkBindBufferMemory(
      //     context->device->GetLogicalDevice(),
      //     native_uniform_buffer->GetBuffer().GetHandle(),
      //     native_uniform_buffer->GetBuffer().GetMemory(), 0));

      builder = builder.BindBuffer(binding.binding, &buffer_info,
                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                   VK_SHADER_STAGE_ALL_GRAPHICS);
    } break;
    case GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE: {
      VulkanTexture *native_texture = (VulkanTexture *)binding.texture;

      VkDescriptorImageInfo image_info = {};
      image_info.sampler = native_texture->GetSampler();
      image_info.imageView = native_texture->GetImageView();
      image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      builder = builder.BindImage(binding.binding, &image_info,
                                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  VK_SHADER_STAGE_ALL_GRAPHICS);
    } break;
    case GPU_DESCRIPTOR_BINDING_TYPE_ATTACHMENT: {
      VulkanAttachment *native_attachment =
          (VulkanAttachment *)binding.attachment;

      VkDescriptorImageInfo image_info = {};
      image_info.sampler = native_attachment->GetSampler();
      image_info.imageView = native_attachment->GetImageView();
      image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      builder = builder.BindImage(binding.binding, &image_info,
                                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  VK_SHADER_STAGE_ALL_GRAPHICS);
    } break;
    }
  }

  builder.Build(&set, &layout);
}

void VulkanDescriptorSet::Destroy() { bindings.clear(); }

void VulkanDescriptorSet::SetDebugName(const char *name) {
  VulkanDebugUtils::SetObjectName(name, (uint64_t)set,
                                  VK_OBJECT_TYPE_DESCRIPTOR_SET);
}

void VulkanDescriptorSet::SetDebugTag(const void *tag, size_t tag_size) {
  VulkanDebugUtils::SetObjectTag(tag, (uint64_t)set,
                                 VK_OBJECT_TYPE_DESCRIPTOR_SET, 0, tag_size);
}
