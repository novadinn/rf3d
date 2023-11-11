#pragma once

#include "renderer/gpu_vertex_buffer.h"
#include "vulkan_buffer.h"

class VulkanVertexBuffer : public GPUVertexBuffer {
public:
  bool Create(uint64_t buffer_size) override;
  void Destroy() override;

  bool Bind(uint64_t offset) override;
  void *Lock(uint64_t offset, uint64_t size) override;
  void Unlock() override;

  bool LoadData(uint64_t offset, uint64_t size, void *data) override;

  uint64_t GetSize() const override;

private:
  VulkanBuffer buffer;
};