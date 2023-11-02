#pragma once

#include "gpu_attachment.h"

#include <stdint.h>
#include <vector>

class GPURenderPass;

class GPURenderTarget {
public:
  virtual ~GPURenderTarget(){};

  virtual bool Create(GPURenderPass *target_render_pass,
                      std::vector<GPUAttachment *> target_attachments,
                      uint32_t target_width, uint32_t target_height) = 0;
  virtual void Destroy() = 0;

  virtual bool Resize(uint32_t new_width, uint32_t new_height) = 0;

  inline std::vector<GPUAttachment *> &GetAttachments() { return attachments; }
  inline uint32_t GetWidth() const { return width; }
  inline uint32_t GetHeight() const { return height; }

protected:
  std::vector<GPUAttachment *> attachments;
  uint32_t width;
  uint32_t height;
};