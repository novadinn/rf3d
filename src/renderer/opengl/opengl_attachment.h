#pragma once

#include "renderer/gpu_attachment.h"

#include <glad/glad.h>

class OpenGLAttachment : public GPUAttachment {
public:
  void Create(GPUFormat image_format, GPUAttachmentUsage texture_usage,
              uint32_t texture_width, uint32_t texture_height) override;
  void Destroy() override;

  inline GLuint GetID() const { return id; }

private:
  GLuint id;
};