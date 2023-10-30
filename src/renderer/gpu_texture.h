#pragma once

#include "gpu_core.h"

#include <stdint.h>

enum GPUTextureAspect {
  GPU_TEXTURE_USAGE_NONE,
  GPU_TEXTURE_USAGE_COLOR_ATTACHMENT,
  GPU_TEXTURE_USAGE_DEPTH_ATTACHMENT,
  GPU_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT,
};

class GPUTexture {
public:
  virtual ~GPUTexture() {}

  virtual void Create(GPUFormat image_format, GPUTextureAspect texture_usage,
                      uint32_t width, uint32_t height) = 0;
  virtual void Destroy() = 0;

  virtual void WriteData(uint8_t *pixels, uint32_t offset) = 0;
  virtual void Resize(uint32_t new_width, uint32_t new_height) = 0;

  inline GPUFormat GetFormat() const { return format; };
  inline GPUTextureAspect GetUsage() const { return aspect; }
  inline uint32_t GetWidth() const { return width; }
  inline uint32_t GetHeight() const { return height; }

protected:
  GPUFormat format;
  GPUTextureAspect aspect;
  uint32_t width;
  uint32_t height;
};