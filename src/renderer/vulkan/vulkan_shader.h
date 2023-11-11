#pragma once

#include "renderer/gpu_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"
#include "vulkan_uniform_buffer.h"

#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

struct VulkanShaderBinding {
  VkDescriptorSetLayoutBinding layout_binding;
  uint64_t size;
};

struct VulkanShaderSet {
  VkDescriptorSetLayout layout;
  /* one per frame */
  std::vector<VkDescriptorSet> sets;
  std::vector<VulkanShaderBinding> bindings;
  uint32_t index;
};

class VulkanContext;

class VulkanShader : public GPUShader {
public:
  bool Create(GPUShaderConfig *config, GPURenderPass *render_pass,
              float viewport_width, float viewport_height) override;
  void Destroy() override;

  void AttachShaderBuffer(GPUUniformBuffer *uniform_buffer, uint32_t set,
                          uint32_t binding) override;

  void Bind() override;
  void BindShaderBuffer(GPUUniformBuffer *uniform_buffer, uint32_t set,
                        uint32_t offset) override;
  void PushConstant(GPUShaderPushConstant *push_constant) override;

  inline VulkanPipeline &GetPipeline() { return pipeline; }

private:
  void ReflectStagePoolSizes(spirv_cross::Compiler &compiler,
                             spirv_cross::ShaderResources &resources,
                             std::vector<VkDescriptorPoolSize> &pool_sizes);
  void ReflectStagePushConstantRanges(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkPushConstantRange> &push_constant_ranges);
  /* TODO: only uniform buffers are supported rn */
  void ReflectStageBuffers(spirv_cross::Compiler &compiler,
                           spirv_cross::ShaderResources &resources,
                           std::vector<VulkanShaderSet> &sets);
  void ReflectVertexAttributes(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkVertexInputAttributeDescription> &attributes,
      uint64_t *out_stride);

  VulkanPipeline pipeline;
  VkDescriptorPool descriptor_pool;
  std::vector<VulkanShaderSet> shader_sets;
};