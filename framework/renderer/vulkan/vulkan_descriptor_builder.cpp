#include "vulkan_descriptor_builder.h"

#include "../../logger.h"
#include "vulkan_backend.h"

VulkanDescriptorBuilder VulkanDescriptorBuilder::Begin() {
  VulkanDescriptorBuilder builder;
  return builder;
}

VulkanDescriptorBuilder &VulkanDescriptorBuilder::BindBuffer(
    uint32_t binding, VkDescriptorBufferInfo *buffer_info,
    VkDescriptorType type, VkShaderStageFlags stage_flags) {
  VkDescriptorSetLayoutBinding layout_binding = {};
  layout_binding.binding = binding;
  layout_binding.descriptorType = type;
  layout_binding.descriptorCount = 1;
  layout_binding.stageFlags = stage_flags;
  layout_binding.pImmutableSamplers = nullptr;

  bindings.emplace_back(layout_binding);

  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.pNext = nullptr;
  /* write_descriptor_set.dstSet; set later */
  write_descriptor_set.dstBinding = binding;
  write_descriptor_set.dstArrayElement = 0;
  write_descriptor_set.descriptorCount = 1;
  write_descriptor_set.descriptorType = type;
  write_descriptor_set.pImageInfo = 0;
  write_descriptor_set.pBufferInfo = buffer_info;
  write_descriptor_set.pTexelBufferView = 0;

  writes.push_back(write_descriptor_set);
  return *this;
}

VulkanDescriptorBuilder &VulkanDescriptorBuilder::BindImage(
    uint32_t binding, VkDescriptorImageInfo *image_info, VkDescriptorType type,
    VkShaderStageFlags stage_flags) {
  VkDescriptorSetLayoutBinding layout_binding = {};
  layout_binding.binding = binding;
  layout_binding.descriptorType = type;
  layout_binding.descriptorCount = 1;
  layout_binding.stageFlags = stage_flags;
  layout_binding.pImmutableSamplers = nullptr;

  bindings.emplace_back(layout_binding);

  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.pNext = nullptr;
  /* write_descriptor_set.dstSet; set later */
  write_descriptor_set.dstBinding = binding;
  write_descriptor_set.dstArrayElement = 0;
  write_descriptor_set.descriptorCount = 1;
  write_descriptor_set.descriptorType = type;
  write_descriptor_set.pImageInfo = image_info;
  write_descriptor_set.pBufferInfo = 0;
  write_descriptor_set.pTexelBufferView = 0;

  writes.push_back(write_descriptor_set);
  return *this;
}

bool VulkanDescriptorBuilder::Build(VkDescriptorSet *out_set,
                                    VkDescriptorSetLayout *out_layout) {
  VulkanContext *context = VulkanBackend::GetContext();

  VkDescriptorSetLayoutCreateInfo layout_info = {};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.pNext = 0;
  layout_info.flags = 0;
  layout_info.pBindings = bindings.data();
  layout_info.bindingCount = bindings.size();

  *out_layout = context->layout_cache->CreateDescriptorLayout(&layout_info);

  *out_set = context->descriptor_pools->Allocate(*out_layout);
  if (!(*out_set)) {
    return false;
  }

  for (VkWriteDescriptorSet &w : writes) {
    w.dstSet = *out_set;
  }

  vkUpdateDescriptorSets(context->device->GetLogicalDevice(), writes.size(),
                         writes.data(), 0, 0);

  return true;
}