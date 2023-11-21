#pragma once

#include "../gpu_texture.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"

#include "vk_mem_alloc.h"
#include <stdint.h>
#include <vulkan/vulkan.h>

class VulkanTexture : public GPUTexture {
public:
  bool Create(GPUFormat texture_format, GPUTextureType texture_type,
              uint32_t texture_width, uint32_t texture_height) override;
  void Destroy() override;

  void WriteData(void *pixels, uint32_t offset) override;

  inline VkImage GetHandle() const { return handle; }
  inline VkImageView GetImageView() const { return view; }
  inline VmaAllocation GetMemory() const { return memory; }
  inline VkSampler GetSampler() const { return sampler; }

private:
  void TransitionLayout(VulkanCommandBuffer *command_buffer, VkFormat format,
                        VkImageLayout old_layout, VkImageLayout new_layout);
  void CopyFromBuffer(VulkanBuffer *buffer, VulkanCommandBuffer *command_buffer,
                      uint64_t offset);
  bool GenerateMipMaps();

  VkImage handle;
  VkImageView view;
  VmaAllocation memory;
  VkSampler sampler;
};