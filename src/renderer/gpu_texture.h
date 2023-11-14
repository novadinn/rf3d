#pragma once

#include "gpu_core.h"

#include <stdint.h>

enum GPUTextureType {
  GPU_TEXTURE_TYPE_NONE,
  GPU_TEXTURE_TYPE_2D,
};

class GPUTexture {
public:
  virtual ~GPUTexture() {}

  virtual void Create(GPUFormat texture_format, GPUTextureType texture_type,
                      uint32_t texture_width, uint32_t texture_height) = 0;
  virtual void Destroy() = 0;

  virtual void WriteData(void *pixels, uint32_t offset) = 0;

  inline GPUFormat GetFormat() const { return format; }
  inline GPUTextureType GetType() const { return type; }
  inline uint32_t GetWidth() const { return width; }
  inline uint32_t GetHeight() const { return height; }

protected:
  GPUFormat format;
  GPUTextureType type;
  uint32_t width;
  uint32_t height;
};