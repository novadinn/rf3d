#pragma once

#include "renderer/gpu_shader_buffer.h"

#include <glad/glad.h>

class OpenGLShaderBuffer : public GPUShaderBuffer {
  void Create(const char *descriptor_name, GPUShaderBufferType descriptor_type,
              uint8_t descriptor_stage_flags, uint64_t descriptor_size,
              uint32_t descriptor_index) override;
  void Destroy() override;

  void Bind(GPUShader *shader) override;

  GPUBuffer *GetBuffer() override;

private:
  GPUBuffer *buffer;
  uint32_t index;
};