#pragma once

#include "renderer/gpu_texture.h"
#include "vulkan_image.h"

#include <stdint.h>

/* TODO: we dont need to have both texture and image classes */
class VulkanTexture : public GPUTexture {
public:
  void Create(GPUFormat image_format, GPUTextureAspect texture_usage,
              uint32_t width, uint32_t height) override;
  void Destroy() override;

  void WriteData(uint8_t *pixels, uint32_t offset) override;
  void Resize(uint32_t new_width, uint32_t new_height) override;

  void SetImage(VulkanImage new_image) { image = new_image; }

  inline VulkanImage &GetImage() { return image; }
  inline VkSampler GetSampler() { return sampler; }

private:
  VulkanImage image;
  /* TODO: we dont need to create a sampler every time */
  VkSampler sampler;
  VkFormat native_format;
  VkImageAspectFlags native_aspect_flags;
};