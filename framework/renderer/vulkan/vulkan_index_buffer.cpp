#include "vulkan_index_buffer.h"

#include "vulkan_backend.h"
#include "vulkan_debug_marker.h"

bool VulkanIndexBuffer::Create(uint64_t buffer_size) {
  return buffer.Create(
      buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
}

void VulkanIndexBuffer::Destroy() { buffer.Destroy(); }

bool VulkanIndexBuffer::Bind(uint64_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  vkCmdBindIndexBuffer(command_buffer->GetHandle(), buffer.GetHandle(), offset,
                       VK_INDEX_TYPE_UINT32);

  return true;
}

void *VulkanIndexBuffer::Lock(uint64_t offset, uint64_t size) {
  return buffer.Lock(offset, size);
}

void VulkanIndexBuffer::Unlock() { buffer.Unlock(); }

bool VulkanIndexBuffer::LoadData(uint64_t offset, uint64_t size, void *data) {
  return buffer.LoadDataStaging(offset, size, data);
}

uint64_t VulkanIndexBuffer::GetSize() const { return buffer.GetSize(); }

void VulkanIndexBuffer::SetDebugName(const char *name) {
  VulkanDebugUtils::SetObjectName(name, (uint64_t)buffer.GetHandle(),
                                  VK_OBJECT_TYPE_BUFFER);
}

void VulkanIndexBuffer::SetDebugTag(const void *tag, size_t tag_size) {
  VulkanDebugUtils::SetObjectTag(tag, (uint64_t)buffer.GetHandle(),
                                 VK_OBJECT_TYPE_BUFFER, 0, tag_size);
}
