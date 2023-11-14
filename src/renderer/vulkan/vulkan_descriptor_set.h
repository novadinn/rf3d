#pragma once

#include "renderer/gpu_descriptor_set.h"

#include <vulkan/vulkan.h>

class VulkanDescriptorSet : public GPUDescriptorSet {
public:
  void Create(uint32_t set_index,
              std::vector<GPUDescriptorBinding> &set_bindings) override;
  void Destroy() override;

  inline std::vector<VkDescriptorSet> &GetSets() { return sets; }
  inline VkDescriptorSetLayout GetLayout() { return layout; }

private:
  /* one per frame (if needed) */
  std::vector<VkDescriptorSet> sets;
  VkDescriptorSetLayout layout;
};