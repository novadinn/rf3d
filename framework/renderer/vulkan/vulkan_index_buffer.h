#pragma once

#include "../gpu_index_buffer.h"
#include "vulkan_buffer.h"

class VulkanIndexBuffer : public GPUIndexBuffer {
public:
  bool Create(uint64_t buffer_size) override;
  void Destroy() override;

  bool Bind(uint64_t offset) override;
  void *Lock(uint64_t offset, uint64_t size) override;
  void Unlock() override;

  bool LoadData(uint64_t offset, uint64_t size, void *data) override;

  uint64_t GetSize() const override;

  void SetDebugName(const char *name) override;
  void SetDebugTag(const void *tag, size_t tag_size) override;

private:
  VulkanBuffer buffer;
};