#include "vulkan_uniform_buffer.h"

#include "vulkan_backend.h"
#include "vulkan_utils.h"

bool VulkanUniformBuffer::Create(GPUShaderBufferIndex buffer_index,
                                 uint64_t buffer_element_size,
                                 uint64_t buffer_element_count) {
  VulkanContext *context = VulkanBackend::GetContext();

  index = buffer_index;
  element_size = buffer_element_size;

  size_t dynamic_alignment = VulkanUtils::GetDynamicAlignment(element_size);
  size_t buffer_size = buffer_element_count * dynamic_alignment;
  uint32_t device_local_bits = context->device->SupportsDeviceLocalHostVisible()
                                   ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                   : 0;

  buffers.resize(context->swapchain->GetImageCount());
  for (int i = 0; i < buffers.size(); ++i) {
    if (!buffers[i].Create(buffer_size,
                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                               device_local_bits)) {
      return false;
    }
  }

  return true;
}

void VulkanUniformBuffer::Destroy() {
  for (int i = 0; i < buffers.size(); ++i) {
    buffers[i].Destroy();
  }
  buffers.clear();
}

void *VulkanUniformBuffer::Lock(uint64_t offset, uint64_t size) {
  VulkanContext *context = VulkanBackend::GetContext();

  return buffers[context->image_index].Lock(offset, size);
}

void VulkanUniformBuffer::Unlock() {
  VulkanContext *context = VulkanBackend::GetContext();

  buffers[context->image_index].Unlock();
}

bool VulkanUniformBuffer::LoadData(uint64_t offset, uint64_t size, void *data) {
  VulkanContext *context = VulkanBackend::GetContext();

  return buffers[context->image_index].LoadData(offset, size, data);
}

uint64_t VulkanUniformBuffer::GetSize() const {
  VulkanContext *context = VulkanBackend::GetContext();

  return buffers[context->image_index].GetSize();
}

uint64_t VulkanUniformBuffer::GetDynamicAlignment() const {
  return VulkanUtils::GetDynamicAlignment(element_size);
}