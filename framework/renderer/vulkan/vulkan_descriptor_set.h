#pragma once

#include "../gpu_descriptor_set.h"

#include <vulkan/vulkan.h>

class VulkanDescriptorSet : public GPUDescriptorSet {
public:
  void Create(std::vector<GPUDescriptorBinding> &set_bindings) override;
  void Destroy() override;

  inline VkDescriptorSet &GetSet() { return set; }
  inline VkDescriptorSetLayout GetLayout() { return layout; }

private:
  VkDescriptorSet set;
  VkDescriptorSetLayout layout;
};