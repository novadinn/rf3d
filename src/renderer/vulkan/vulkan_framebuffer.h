#pragma once

#include "renderer/gpu_render_target.h"

#include <vulkan/vulkan.h>

class VulkanContext;
class VulkanRenderPass;

class VulkanFramebuffer : public GPURenderTarget {
public:
  bool Create(GPURenderPass *target_render_pass,
              std::vector<GPUTexture *> target_attachments,
              uint32_t target_width, uint32_t target_height) override;
  void Destroy() override;

  bool Resize(uint32_t new_width, uint32_t new_height) override;

  inline VkFramebuffer &GetHandle() { return handle; }

private:
  VkFramebuffer handle;
};