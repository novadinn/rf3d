#include "vulkan_attribute_array.h"

void VulkanAttributeArray::Create(GPUBuffer *target_vertex_buffer,
                                  GPUBuffer *target_index_buffer,
                                  std::vector<GPUFormat> &attribute_formats) {
  vertex_buffer = target_vertex_buffer;
  index_buffer = target_index_buffer;
}

void VulkanAttributeArray::Destroy() {
  vertex_buffer = 0;
  index_buffer = 0;
}

void VulkanAttributeArray::Bind() {
  if (index_buffer) {
    index_buffer->Bind(0);
  } else {
    vertex_buffer->Bind(0);
  }
}