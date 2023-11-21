#include "vulkan_buffer.h"

#include "../../logger.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

#include <string.h>

bool VulkanBuffer::Create(uint64_t buffer_size, VkBufferUsageFlags usage_flags,
                          VkMemoryPropertyFlags memory_flags,
                          VmaMemoryUsage vma_usage) {
  VulkanContext *context = VulkanBackend::GetContext();

  total_size = buffer_size;

  VkBufferCreateInfo buffer_create_info = {};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.pNext = 0;
  buffer_create_info.flags = 0;
  buffer_create_info.size = total_size;
  buffer_create_info.usage = usage_flags;
  /* TODO: only used in one queue. */
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_create_info.queueFamilyIndexCount = 0;
  buffer_create_info.pQueueFamilyIndices = 0;

  VmaAllocationCreateInfo vma_allocation_create_info = {};
  /* vma_allocation_create_info.flags; */
  vma_allocation_create_info.usage = vma_usage;
  // vma_allocation_create_info.requiredFlags = memory_flags;
  /* vma_allocation_create_info.preferredFlags;
  vma_allocation_create_info.memoryTypeBits;
  vma_allocation_create_info.pool;
  vma_allocation_create_info.pUserData;
  vma_allocation_create_info.priority; */

  VK_CHECK(vmaCreateBuffer(context->vma_allocator, &buffer_create_info,
                           &vma_allocation_create_info, &handle, &memory, 0));

  return true;
}

void VulkanBuffer::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  vmaDestroyBuffer(context->vma_allocator, handle, memory);

  handle = 0;
  memory = 0;
  total_size = 0;
}

void *VulkanBuffer::Lock(uint64_t offset, uint64_t size) {
  VulkanContext *context = VulkanBackend::GetContext();

  void *data;
  VK_CHECK(vmaMapMemory(context->vma_allocator, memory, &data));
  return data;
}

void VulkanBuffer::Unlock() {
  VulkanContext *context = VulkanBackend::GetContext();

  vmaUnmapMemory(context->vma_allocator, memory);
}

bool VulkanBuffer::LoadData(uint64_t offset, uint64_t size, void *data) {
  VulkanContext *context = VulkanBackend::GetContext();

  void *data_ptr = Lock(offset, size);
  memcpy(data_ptr, data, size);
  Unlock();

  return true;
}

bool VulkanBuffer::LoadDataStaging(uint64_t offset, uint64_t size, void *data) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanBuffer staging_buffer;
  if (!staging_buffer.Create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             VMA_MEMORY_USAGE_CPU_ONLY)) {
    ERROR("Failed to create a staging buffer.");
    return false;
  }

  staging_buffer.LoadData(0, staging_buffer.GetSize(), data);
  // VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(),
  //                             staging_buffer.GetHandle(),
  //                             staging_buffer.GetMemory(), 0));
  // VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(), handle,
  //                             memory, 0));

  staging_buffer.CopyTo(this, 0, 0, staging_buffer.GetSize());

  staging_buffer.Destroy();

  return true;
}

bool VulkanBuffer::CopyTo(VulkanBuffer *dest, uint64_t source_offset,
                          uint64_t dest_offset, uint64_t size) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo graphics_queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VkQueue queue = graphics_queue_info.queue;
  VkCommandPool command_pool = graphics_queue_info.command_pool;

  vkQueueWaitIdle(queue);
  VulkanCommandBuffer temp_command_buffer;
  temp_command_buffer.AllocateAndBeginSingleUse(command_pool);

  VkBufferCopy copy_region;
  copy_region.srcOffset = source_offset;
  copy_region.dstOffset = dest_offset;
  copy_region.size = size;

  vkCmdCopyBuffer(temp_command_buffer.GetHandle(), handle, dest->GetHandle(), 1,
                  &copy_region);

  temp_command_buffer.FreeAndEndSingleUse(command_pool, queue);

  return true;
}