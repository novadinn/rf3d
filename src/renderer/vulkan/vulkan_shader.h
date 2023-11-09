#pragma once

#include "renderer/gpu_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"

#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanContext;

class VulkanShader : public GPUShader {
public:
  bool Create(GPUShaderConfig *config, GPURenderPass *render_pass,
              float viewport_width, float viewport_height) override;
  void Destroy() override;

  /* TODO: we dont need these function at all, we can just use reflection to get
   * the correct buffer sizes */
  void PrepareShaderBuffer(GPUShaderBufferIndex index, uint64_t size,
                           uint32_t element_count) override;
  GPUBuffer *GetShaderBuffer(GPUShaderBufferIndex index) override;

  void Bind() override;
  void BindShaderBuffer(GPUShaderBufferIndex index, uint32_t draw_index,
                        uint64_t size) override;
  void PushConstant(GPUShaderPushConstant *push_constant) override;
  void SetTexture(uint32_t index, GPUTexture *texture) override;

  inline VulkanPipeline &GetPipeline() { return pipeline; }

private:
  struct VulkanShaderBuffer {
    VkDescriptorSetLayout layout;
    std::vector<VkDescriptorSet> sets;
    std::vector<VulkanBuffer> buffers;
  };

  void ReflectStagePoolSizes(spirv_cross::Compiler &compiler,
                             spirv_cross::ShaderResources &resources,
                             std::vector<VkDescriptorPoolSize> &pool_sizes);
  void ReflectStagePushConstantRanges(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkPushConstantRange> &push_constant_ranges);
  /* TODO: only uniform buffers are supported rn */
  void ReflectStageBuffers(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>
          &sets_and_bindings);
  void ReflectVertexAttributes(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkVertexInputAttributeDescription> &attributes,
      uint64_t *out_stride);

  VulkanPipeline pipeline;
  VkDescriptorPool descriptor_pool;
  /* TODO: instead of storing it here, it is better to have a global buffer
   * storage */
  std::unordered_map<GPUShaderBufferIndex, VulkanShaderBuffer> uniform_buffers;
};