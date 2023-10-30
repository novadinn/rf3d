#pragma once

#include "renderer/gpu_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanContext;

struct VulkanShaderDescriptor {
  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout descriptor_set_layout;
  std::vector<VkDescriptorSet> descriptor_sets;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  std::vector<VulkanBuffer> buffers;
};

class VulkanShader : public GPUShader {
public:
  bool Create(GPUShaderConfig *config, GPURenderPass *render_pass,
              float viewport_width, float viewport_height) override;
  void Destroy() override;

  void Bind() override;

  GPUBuffer *GetDescriptorBuffer(const char *name) override;

  inline VulkanPipeline &GetPipeline() { return pipeline; }

private:
  /* TODO: do we really need to create a separate structure for a pipeline */
  VulkanPipeline pipeline;
  std::unordered_map<const char *, VulkanShaderDescriptor> uniform_buffers;
};