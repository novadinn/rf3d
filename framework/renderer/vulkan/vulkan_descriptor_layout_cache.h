#pragma once

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanDescriptorLayoutCache {
public:
  void Initialize();
  void Shutdown();

  VkDescriptorSetLayout
  CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo *layout_create_info);

  struct DescriptorLayoutInfo {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    bool operator==(const DescriptorLayoutInfo &other) const;
    size_t hash() const;
  };

private:
  struct DescriptorLayoutHash {
    std::size_t operator()(const DescriptorLayoutInfo &info) const {
      return info.hash();
    }
  };

  std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout,
                     DescriptorLayoutHash>
      layout_cache;
};