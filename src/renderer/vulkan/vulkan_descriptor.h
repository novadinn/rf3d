#pragma once

#include "renderer/gpu_descriptor.h"

#include <vector>
#include <vulkan/vulkan.h>

class VulkanDescriptor : public GPUDescriptor {
public:
  void Create(const char *descriptor_name, GPUDescriptorType descriptor_type,
              uint8_t descriptor_stage_flags, uint64_t descriptor_size,
              uint32_t index) override;
  void Destroy() override;

  void Bind(GPUShader *shader) override;

  GPUBuffer *GetBuffer() override;

  inline VkDescriptorSetLayout GetSetLayout() { return set_layout; }

private:
  VkDescriptorPool pool;
  VkDescriptorSetLayout set_layout;
  std::vector<VkDescriptorSet> sets;
  std::vector<GPUBuffer *> buffers;
};