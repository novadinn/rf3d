#pragma once

#include "../gpu_attachment.h"

#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>

class VulkanAttachment : public GPUAttachment {
public:
  void Create(GPUFormat attachment_format, GPUAttachmentUsage attachment_usage,
              uint32_t attachment_width, uint32_t attachment_height) override;
  void Destroy() override;

  void SetDebugName(const char *name) override;
  void SetDebugTag(const void *tag, size_t tag_size) override;

  void CreateAsSwapchainAttachment(VkImage new_handle, VkImageView new_view);
  void DestroyAsSwapchainAttachment();

  inline VkSampler GetSampler() { return sampler; }
  inline VkImageView GetImageView() const { return view; }

private:
  VkImage handle;
  VkImageView view;
  VmaAllocation memory;
  VkSampler sampler;
};