#pragma once

#include "renderer/gpu_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"

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
  void PushConstant(void *value, uint64_t size, uint32_t offset,
                    uint8_t stage_flags) override;

  inline VulkanPipeline &GetPipeline() { return pipeline; }

private:
  VulkanPipeline pipeline;
};