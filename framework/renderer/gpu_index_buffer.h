#pragma once

#include <stdint.h>

class GPUIndexBuffer {
public:
  virtual ~GPUIndexBuffer(){};

  virtual bool Create(uint64_t buffer_size) = 0;
  virtual void Destroy() = 0;

  virtual bool Bind(uint64_t offset) = 0;
  virtual void *Lock(uint64_t offset, uint64_t size) = 0;
  virtual void Unlock() = 0;

  virtual bool LoadData(uint64_t offset, uint64_t size, void *data) = 0;

  virtual uint64_t GetSize() const = 0;
};