#include "vulkan_descriptor_set.h"

#include "../../logger.h"
#include "vulkan_backend.h"
#include "vulkan_debug_marker.h"
#include "vulkan_descriptor_builder.h"
#include "vulkan_texture.h"
#include "vulkan_uniform_buffer.h"

#include <vector>

void VulkanDescriptorSet::Create(
    std::vector<GPUDescriptorBinding> &set_bindings) {
  VulkanContext *context = VulkanBackend::GetContext();

  bindings = set_bindings;

  VulkanDescriptorBuilder builder = VulkanDescriptorBuilder::Begin();

  /* TODO: seriosly? */
  std::vector<VkDescriptorBufferInfo> uniform_buffers_info;
  uniform_buffers_info.resize(bindings.size());
  uint32_t uniform_buffer_count = 0;
  std::vector<VkDescriptorImageInfo> texture_infos;
  texture_infos.resize(bindings.size());
  uint32_t texture_count = 0;
  std::vector<VkDescriptorImageInfo> attachment_infos;
  attachment_infos.resize(bindings.size());
  uint32_t attachment_count = 0;

  for (uint32_t i = 0; i < bindings.size(); ++i) {
    GPUDescriptorBinding &binding = bindings[i];
    switch (binding.type) {
    case GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER: {
      VulkanUniformBuffer *native_uniform_buffer =
          (VulkanUniformBuffer *)binding.uniform_buffer;

      uniform_buffers_info[uniform_buffer_count].buffer =
          native_uniform_buffer->GetBuffer().GetHandle();
      uniform_buffers_info[uniform_buffer_count].offset = 0;
      uniform_buffers_info[uniform_buffer_count].range =
          native_uniform_buffer->GetDynamicAlignment();

      // VK_CHECK(vkBindBufferMemory(
      //     context->device->GetLogicalDevice(),
      //     native_uniform_buffer->GetBuffer().GetHandle(),
      //     native_uniform_buffer->GetBuffer().GetMemory(), 0));

      builder = builder.BindBuffer(binding.binding,
                                   &uniform_buffers_info[uniform_buffer_count],
                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                   VK_SHADER_STAGE_ALL_GRAPHICS);
      ++uniform_buffer_count;
    } break;
    case GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE: {
      VulkanTexture *native_texture = (VulkanTexture *)binding.texture;

      texture_infos[texture_count].sampler = native_texture->GetSampler();
      texture_infos[texture_count].imageView = native_texture->GetImageView();
      texture_infos[texture_count].imageLayout =
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      builder =
          builder.BindImage(binding.binding, &texture_infos[texture_count],
                            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            VK_SHADER_STAGE_ALL_GRAPHICS);
      texture_count++;
    } break;
    case GPU_DESCRIPTOR_BINDING_TYPE_ATTACHMENT: {
      VulkanAttachment *native_attachment =
          (VulkanAttachment *)binding.attachment;

      attachment_infos[attachment_count].sampler =
          native_attachment->GetSampler();
      attachment_infos[attachment_count].imageView =
          native_attachment->GetImageView();
      attachment_infos[attachment_count].imageLayout =
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      builder = builder.BindImage(binding.binding,
                                  &attachment_infos[attachment_count],
                                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  VK_SHADER_STAGE_ALL_GRAPHICS);
      attachment_count++;
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
