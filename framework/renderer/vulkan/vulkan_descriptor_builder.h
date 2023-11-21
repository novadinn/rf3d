#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VulkanDescriptorBuilder {
public:
  static VulkanDescriptorBuilder Begin();

  VulkanDescriptorBuilder &BindBuffer(uint32_t binding,
                                      VkDescriptorBufferInfo *buffer_info,
                                      VkDescriptorType type,
                                      VkShaderStageFlags stage_flags);
  VulkanDescriptorBuilder &BindImage(uint32_t binding,
                                     VkDescriptorImageInfo *image_info,
                                     VkDescriptorType type,
                                     VkShaderStageFlags stage_flags);

  bool Build(VkDescriptorSet *out_set, VkDescriptorSetLayout *out_layout);

private:
  std::vector<VkWriteDescriptorSet> writes;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
};