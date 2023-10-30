#pragma once

#include <stdint.h>

enum GPUBufferType {
  GPU_BUFFER_TYPE_VERTEX,
  GPU_BUFFER_TYPE_INDEX,
  GPU_BUFFER_TYPE_UNIFORM,
  GPU_BUFFER_TYPE_STAGING,
};

class GPUBuffer {
public:
  virtual ~GPUBuffer(){};

  virtual bool Create(GPUBufferType buffer_type, uint64_t buffer_size) = 0;
  virtual void Destroy() = 0;

  virtual bool Bind(uint64_t offset) = 0;
  virtual void *Lock(uint64_t offset, uint64_t size) = 0;
  virtual void Unlock() = 0;

  virtual bool LoadData(uint64_t offset, uint64_t size, void *data) = 0;
  virtual bool CopyTo(GPUBuffer *dest, uint64_t source_offset,
                      uint64_t dest_offset, uint64_t size) = 0;

  inline GPUBufferType GetType() const { return type; }
  inline uint64_t GetSize() const { return total_size; }

protected:
  GPUBufferType type;
  uint64_t total_size;
};