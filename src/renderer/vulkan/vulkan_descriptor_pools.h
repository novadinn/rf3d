#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VulkanDescriptorPools {
public:
  void Initialize();
  void Shutdown();

  void Reset();

  VkDescriptorSet Allocate(VkDescriptorSetLayout layout);
  void Free(VkDescriptorSet descriptor_set);

private:
  VkDescriptorPool GrabPool();
  VkDescriptorPool current_pool;
  std::vector<VkDescriptorPool> used_pools;
  std::vector<VkDescriptorPool> free_pools;
};