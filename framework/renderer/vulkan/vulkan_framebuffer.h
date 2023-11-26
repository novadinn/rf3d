#pragma once

#include "../gpu_render_target.h"

#include <vulkan/vulkan.h>

class VulkanContext;
class VulkanRenderPass;

class VulkanFramebuffer : public GPURenderTarget {
public:
  bool Create(GPURenderPass *target_render_pass,
              std::vector<GPUAttachment *> target_attachments,
              uint32_t target_width, uint32_t target_height) override;
  void Destroy() override;

  bool Resize(uint32_t new_width, uint32_t new_height) override;

  void SetDebugName(const char *name) override;
  void SetDebugTag(const void *tag, size_t tag_size) override;

  inline VkFramebuffer &GetHandle() { return handle; }

private:
  VkFramebuffer handle;
};