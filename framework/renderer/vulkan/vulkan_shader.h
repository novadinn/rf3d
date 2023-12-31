#pragma once

#include "../gpu_descriptor_set.h"
#include "../gpu_shader.h"
#include "../gpu_texture.h"
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
  bool Create(GPUShaderConfig * config) override;
  void Destroy() override;

  void Bind() override;
  void BindUniformBuffer(GPUDescriptorSet *set, uint32_t offset,
                         int32_t set_index) override;
  void BindSampler(GPUDescriptorSet *set, int32_t set_index) override;
  void PushConstant(void *value, uint64_t size, uint32_t offset,
                    uint8_t stage_flags) override;

  void SetDebugName(const char *name) override;
  void SetDebugTag(const void *tag, size_t tag_size) override;

  inline VulkanPipeline &GetPipeline() { return pipeline; }

private:
  /* used for reflection. TODO: just use unordered_map instead */
  struct VulkanShaderSet {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    uint32_t index;
  };

  void ReflectStagePushConstantRanges(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkPushConstantRange> &push_constant_ranges);
  /* TODO: for some reason, this function returns incorrect results when shader
   * sets are not places in the increasing order */
  void ReflectStageUniforms(spirv_cross::Compiler &compiler,
                            spirv_cross::ShaderResources &resources,
                            std::vector<VulkanShaderSet> &sets);
  bool ReflectVertexAttributes(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkVertexInputAttributeDescription> &attributes,
      uint64_t *out_stride);
  uint32_t ReflectFragmentOutputs(spirv_cross::Compiler &compiler,
                                  spirv_cross::ShaderResources &resources);
  uint32_t
  ReflectTesselationControlPoints(spirv_cross::Compiler &compiler,
                                  spirv_cross::ShaderResources &resources);
  bool UpdateDescriptorSetsReflection(std::vector<VulkanShaderSet> &sets,
                                      uint32_t set, uint32_t binding,
                                      int32_t *out_set_index);

  VulkanPipeline pipeline;
};