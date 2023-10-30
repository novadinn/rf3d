#pragma once

#include "renderer/gpu_texture.h"

#include <glad/glad.h>

class OpenGLTexture : public GPUTexture {
public:
  void Create(GPUFormat image_format, GPUTextureAspect texture_usage,
              uint32_t width, uint32_t height) override;
  void Destroy() override;

  void WriteData(uint8_t *pixels, uint32_t offset) override;
  void Resize(uint32_t new_width, uint32_t new_height) override;

  inline GLuint GetID() const { return id; }

private:
  GLuint id;
};