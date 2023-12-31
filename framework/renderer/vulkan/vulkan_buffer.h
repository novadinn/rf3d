#pragma once

#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>

class VulkanBuffer {
public:
  bool Create(uint64_t buffer_size, VkBufferUsageFlags usage_flags,
              VkMemoryPropertyFlags memory_flags, VmaMemoryUsage vma_usage);
  void Destroy();

  void *Lock(uint64_t offset, uint64_t size);
  void Unlock();

  bool LoadData(uint64_t offset, uint64_t size, void *data);
  bool LoadDataStaging(uint64_t offset, uint64_t size, void *data);
  bool CopyTo(VulkanBuffer *dest, uint64_t source_offset, uint64_t dest_offset,
              uint64_t size);

  inline VkBuffer &GetHandle() { return handle; }
  inline VmaAllocation GetMemory() { return memory; }
  inline uint64_t GetSize() const { return total_size; }

private:
  VkBuffer handle;
  VmaAllocation memory;
  uint64_t total_size;
};