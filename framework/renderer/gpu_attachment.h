#pragma once

#include "gpu_core.h"

#include <stdint.h>
#include <stdio.h>

enum GPUAttachmentUsage {
  GPU_ATTACHMENT_USAGE_NONE,
  GPU_ATTACHMENT_USAGE_COLOR_ATTACHMENT,
  GPU_ATTACHMENT_USAGE_DEPTH_ATTACHMENT,
  GPU_ATTACHMENT_USAGE_DEPTH_STENCIL_ATTACHMENT,
};

class GPUAttachment {
public:
  virtual ~GPUAttachment() {}

  virtual void Create(GPUFormat attachment_format,
                      GPUAttachmentUsage attachment_usage,
                      uint32_t texture_width, uint32_t texture_height) = 0;
  virtual void Destroy() = 0;

  virtual void SetDebugName(const char *name) = 0;
  virtual void SetDebugTag(const void *tag, size_t tag_size) = 0;

  inline GPUFormat GetFormat() const { return format; }
  inline GPUAttachmentUsage GetUsage() const { return aspect; }
  inline uint32_t GetWidth() const { return width; }
  inline uint32_t GetHeight() const { return height; }

protected:
  GPUFormat format;
  GPUAttachmentUsage aspect;
  uint32_t width;
  uint32_t height;
};