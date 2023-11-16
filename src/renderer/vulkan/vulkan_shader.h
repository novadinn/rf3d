#pragma once

#include "renderer/gpu_descriptor_set.h"
#include "renderer/gpu_shader.h"
#include "renderer/gpu_texture.h"
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"
#include "vulkan_uniform_buffer.h"

#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanContext;

class VulkanShader : public GPUShader {
public:
  bool Create(std::vector<GPUShaderStageConfig> stage_configs,
              GPUShaderTopologyType topology_type, GPURenderPass *render_pass,
              float viewport_width, float viewport_height) override;
  void Destroy() override;

  void Bind() override;
  void BindUniformBuffer(GPUDescriptorSet *set, uint32_t offset) override;
  void BindSampler(GPUDescriptorSet *set) override;
  void PushConstant(void *value, uint64_t size, uint32_t offset,
                    uint8_t stage_flags) override;

  inline VulkanPipeline &GetPipeline() { return pipeline; }

private:
  /* used for reflection */
  struct VulkanShaderBinding {
    VkDescriptorSetLayoutBinding layout_binding;
    uint64_t size;
  };

  /* used for reflection */
  struct VulkanShaderSet {
    std::vector<VulkanShaderBinding> bindings;
    uint32_t index;
  };

  void ReflectStagePushConstantRanges(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkPushConstantRange> &push_constant_ranges);
  void ReflectStageUniforms(spirv_cross::Compiler &compiler,
                            spirv_cross::ShaderResources &resources,
                            std::vector<VulkanShaderSet> &sets);
  void ReflectVertexAttributes(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkVertexInputAttributeDescription> &attributes,
      uint64_t *out_stride);
  bool UpdateDescriptorSetsReflection(std::vector<VulkanShaderSet> &sets,
                                      uint32_t set, uint32_t binding,
                                      int32_t *out_set_index);

  VulkanPipeline pipeline;
};