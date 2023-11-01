#pragma once

#include "renderer/gpu_shader_buffer.h"

#include <vector>
#include <vulkan/vulkan.h>

class VulkanShaderBuffer : public GPUShaderBuffer {
public:
  void Create(const char *descriptor_name, GPUShaderBufferType descriptor_type,
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