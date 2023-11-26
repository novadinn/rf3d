#pragma once

#include <stdint.h>
#include <stdio.h>

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

  virtual void SetDebugName(const char *name) = 0;
  virtual void SetDebugTag(const void *tag, size_t tag_size) = 0;
};