#pragma once

#include "renderer/gpu_render_pass.h"

#include <vulkan/vulkan.h>

class VulkanContext;
class VulkanCommandBuffer;

class VulkanRenderPass : public GPURenderPass {
public:
  void
  Create(std::vector<GPURenderPassAttachmentConfig> pass_render_attachments,
         glm::vec4 pass_render_area, glm::vec4 pass_clear_color,
         float pass_depth, float pass_stencil,
         uint8_t pass_clear_flags) override;
  void Destroy() override;

  void Begin(GPURenderTarget *target) override;
  void End() override;

  inline VkRenderPass GetHandle() const { return handle; }

private:
  VkRenderPass handle;
  float depth;
  float stencil;
};