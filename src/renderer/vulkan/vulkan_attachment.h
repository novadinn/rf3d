#pragma once

#include "renderer/gpu_attachment.h"

#include <vulkan/vulkan.h>

class VulkanAttachment : public GPUAttachment {
public:
  void Create(GPUFormat attachment_format, GPUAttachmentUsage attachment_usage,
              uint32_t texture_width, uint32_t texture_height) override;
  void Destroy() override;

  void SetImageView(VkImageView new_view) { /* TODO: copy construcor */
    view = new_view;
  }

private:
  VkImage handle;
  VkImageView view;
  VkDeviceMemory memory;
};