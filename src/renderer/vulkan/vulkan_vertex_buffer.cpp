#include "vulkan_vertex_buffer.h"

#include "vulkan_backend.h"

bool VulkanVertexBuffer::Create(uint64_t buffer_size) {
  return buffer.Create(
      buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
}

void VulkanVertexBuffer::Destroy() { buffer.Destroy(); }

bool VulkanVertexBuffer::Bind(uint64_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  VkBuffer vertex_buffers[] = {buffer.GetHandle()};
  VkDeviceSize offsets[] = {offset};

  vkCmdBindVertexBuffers(command_buffer->GetHandle(), 0, 1, vertex_buffers,
                         offsets);

  return true;
}

void *VulkanVertexBuffer::Lock(uint64_t offset, uint64_t size) {
  return buffer.Lock(offset, size);
}

void VulkanVertexBuffer::Unlock() { buffer.Unlock(); }

bool VulkanVertexBuffer::LoadData(uint64_t offset, uint64_t size, void *data) {
  return buffer.LoadDataStaging(offset, size, data);
}

uint64_t VulkanVertexBuffer::GetSize() const { return buffer.GetSize(); }