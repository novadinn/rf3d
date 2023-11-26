#pragma once

#include "../gpu_uniform_buffer.h"
#include "vulkan_buffer.h"

#include <stdint.h>
#include <vector>

class VulkanUniformBuffer : public GPUUniformBuffer {
public:
  bool Create(uint64_t buffer_element_size,
              uint64_t buffer_element_count = 1) override;
  void Destroy() override;

  void *Lock(uint64_t offset, uint64_t size) override;
  void Unlock() override;

  bool LoadData(uint64_t offset, uint64_t size, void *data) override;

  void SetDebugName(const char *name) override;
  void SetDebugTag(const void *tag, size_t tag_size) override;

  inline uint64_t GetSize() const override;
  inline VulkanBuffer &GetBuffer() { return buffer; }

private:
  VulkanBuffer buffer;
};