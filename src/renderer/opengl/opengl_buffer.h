#pragma once

#include "renderer/gpu_buffer.h"

#include <glad/glad.h>

class OpenGLBuffer : public GPUBuffer {
public:
  bool Create(GPUBufferType buffer_type, uint64_t buffer_size) override;
  void Destroy() override;
  bool Bind(uint64_t offset) override;
  void *Lock(uint64_t offset, uint64_t size) override;
  void Unlock() override;
  bool LoadData(uint64_t offset, uint64_t size, void *data) override;
  bool CopyTo(GPUBuffer *dest, uint64_t source_offset, uint64_t dest_offset,
              uint64_t size) override;

  inline GLenum GetInternalType() const { return internal_type; }
  inline GLuint GetID() const { return id; }

private:
  GLuint id;
  GLenum internal_type;
};