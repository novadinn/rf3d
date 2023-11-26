#include "vulkan_uniform_buffer.h"

#include "vulkan_backend.h"
#include "vulkan_debug_marker.h"
#include "vulkan_utils.h"

bool VulkanUniformBuffer::Create(uint64_t buffer_element_size,
                                 uint64_t buffer_element_count) {
  VulkanContext *context = VulkanBackend::GetContext();

  dynamic_alignment = VulkanUtils::GetDynamicAlignment(buffer_element_size);
  size_t buffer_size = buffer_element_count * dynamic_alignment;

  uint32_t device_local_bits = context->device->SupportsDeviceLocalHostVisible()
                                   ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                   : 0;

  /* TODO: vmaCreateBufferWithAlignment()? */
  if (!buffer.Create(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                         device_local_bits,
                     VMA_MEMORY_USAGE_CPU_TO_GPU)) {
    return false;
  }

  return true;
}

void VulkanUniformBuffer::Destroy() { buffer.Destroy(); }

void *VulkanUniformBuffer::Lock(uint64_t offset, uint64_t size) {
  return buffer.Lock(offset, size);
}

void VulkanUniformBuffer::Unlock() { buffer.Unlock(); }

bool VulkanUniformBuffer::LoadData(uint64_t offset, uint64_t size, void *data) {
  return buffer.LoadData(offset, size, data);
}

void VulkanUniformBuffer::SetDebugName(const char *name) {
  VulkanDebugUtils::SetObjectName(name, (uint64_t)buffer.GetHandle(),
                                  VK_OBJECT_TYPE_BUFFER);
}

void VulkanUniformBuffer::SetDebugTag(const void *tag, size_t tag_size) {
  VulkanDebugUtils::SetObjectTag(tag, (uint64_t)buffer.GetHandle(),
                                 VK_OBJECT_TYPE_BUFFER, 0, tag_size);
}

uint64_t VulkanUniformBuffer::GetSize() const { return buffer.GetSize(); }