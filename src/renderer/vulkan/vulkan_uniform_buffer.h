#pragma once

#include "renderer/gpu_uniform_buffer.h"
#include "vulkan_buffer.h"

#include <stdint.h>
#include <vector>

class VulkanUniformBuffer : public GPUUniformBuffer {
public:
  bool Create(GPUShaderBufferIndex buffer_index, uint64_t buffer_element_size,
              uint64_t buffer_element_count = 1) override;
  void Destroy() override;

  void *Lock(uint64_t offset, uint64_t size) override;
  void Unlock() override;

  bool LoadData(uint64_t offset, uint64_t size, void *data) override;

  uint64_t GetSize() const override;
  uint64_t GetDynamicAlignment() const override;

  VulkanBuffer GetBufferAtFrame(uint32_t frame) const { return buffers[frame]; }

private:
  /* one per frame */
  std::vector<VulkanBuffer> buffers;
};