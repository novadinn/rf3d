#pragma once

#include "renderer/gpu_texture.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"

#include <stdint.h>
#include <vulkan/vulkan.h>

/* TODO: we dont need to have both texture and image classes */
class VulkanTexture : public GPUTexture {
public:
  void Create(GPUFormat texture_format, GPUTextureType texture_type,
              uint32_t texture_width, uint32_t texture_height) override;
  void Destroy() override;

  void WriteData(uint8_t *pixels, uint32_t offset) override;

  void TransitionLayout(VulkanCommandBuffer *command_buffer, VkFormat format,
                        VkImageLayout old_layout, VkImageLayout new_layout);
  void CopyFromBuffer(VulkanBuffer *buffer,
                      VulkanCommandBuffer *command_buffer);

  inline VkImageView GetImageView() { return view; }

private:
  VkImage handle;
  VkImageView view;
  VkDeviceMemory memory;
};