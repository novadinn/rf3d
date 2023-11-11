#pragma once

#include "gpu_core.h"

#include <stdint.h>

class GPUUniformBuffer {
public:
  virtual ~GPUUniformBuffer(){};

  virtual bool Create(uint64_t buffer_element_size,
                      uint64_t buffer_element_count = 1) = 0;
  virtual void Destroy() = 0;

  virtual void *Lock(uint64_t offset, uint64_t size) = 0;
  virtual void Unlock() = 0;

  virtual bool LoadData(uint64_t offset, uint64_t size, void *data) = 0;

  virtual uint64_t GetSize() const = 0;

  inline uint64_t GetDynamicAlignment() const { return dynamic_alignment; }

protected:
  uint64_t dynamic_alignment;
};