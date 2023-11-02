#pragma once

#include "renderer/gpu_texture.h"

#include <glad/glad.h>

class OpenGLTexture : public GPUTexture {
public:
  void Create(GPUFormat texture_format, GPUTextureType texture_type,
              uint32_t texture_width, uint32_t texture_height) override;
  void Destroy() override;

  void WriteData(uint8_t *pixels, uint32_t offset) override;

  inline GLuint GetID() const { return id; }

private:
  GLuint id;
};