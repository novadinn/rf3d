#pragma once

#include "renderer/gpu_texture.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"

#include <stdint.h>
#include <vulkan/vulkan.h>

/* TODO: we dont need to have both texture and image classes */
class VulkanTexture : public GPUTexture {
public:
  void Create(GPUFormat image_format, GPUTextureUsage texture_usage,
              uint32_t width, uint32_t height) override;
  void Destroy() override;

  void WriteData(uint8_t *pixels, uint32_t offset) override;
  void Resize(uint32_t new_width, uint32_t new_height) override;

  void TransitionLayout(VulkanCommandBuffer *command_buffer, VkFormat format,
                        VkImageLayout old_layout, VkImageLayout new_layout);
  void CopyFromBuffer(VulkanBuffer *buffer,
                      VulkanCommandBuffer *command_buffer);

  void SetImageView(VkImageView new_view) { view = new_view; }
  inline VkImageView GetImageView() { return view; }

private:
  VkImage handle;
  VkImageView view;
  VkDeviceMemory memory;
  /* TODO: sampler */
  VkFormat native_format;
  VkImageAspectFlags native_aspect_flags;
};