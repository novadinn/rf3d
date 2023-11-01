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

  inline VulkanPipeline &GetPipeline() { return pipeline; }

private:
  VulkanPipeline pipeline;
};