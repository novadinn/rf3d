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

  void Bind() override;
  void PushConstant(GPUShaderPushConstant *push_constant) override;
  void SetTexture(uint32_t index, GPUTexture *texture) override;

  inline VulkanPipeline &GetPipeline() { return pipeline; }

private:
  void GetStagePoolSizes(spirv_cross::Compiler &compiler,
                         spirv_cross::ShaderResources &resources,
                         std::vector<VkDescriptorPoolSize> &pool_sizes);
  void GetStagePushConstantRanges(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkPushConstantRange> &push_constant_ranges);
  void GetVertexAttributes(
      spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
      std::vector<VkVertexInputAttributeDescription> &attributes,
      uint64_t *out_stride);

  VulkanPipeline pipeline;
  VkDescriptorPool pool;
};