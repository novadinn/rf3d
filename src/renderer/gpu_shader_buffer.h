#pragma once

#include "gpu_buffer.h"

#include <stdint.h>

class GPUShader;

enum GPUShaderBufferType {
  GPU_DESCRIPTOR_TYPE_NONE,
  GPU_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
};

class GPUShaderBuffer {
public:
  virtual ~GPUShaderBuffer() {}
  virtual void Create(const char *descriptor_name,
                      GPUShaderBufferType descriptor_type,
                      uint8_t descriptor_stage_flags, uint64_t descriptor_size,
                      uint32_t descriptor_index) = 0;
  virtual void Destroy() = 0;

  virtual void Bind(GPUShader *shader) = 0;

  virtual GPUBuffer *GetBuffer() = 0;

  inline const char *GetName() { return name; }

protected:
  const char *name;
  GPUShaderBufferType type;
};