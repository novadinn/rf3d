#pragma once

#include "renderer/gpu_attribute_array.h"

#include <vulkan/vulkan.h>

class VulkanAttributeArray : public GPUAttributeArray {
public:
  void Create(GPUBuffer *attribute_vertex_buffer,
              GPUBuffer *attribute_index_buffer,
              std::vector<GPUFormat> &attribute_formats) override;
  void Destroy() override;

  void Bind() override;

protected:
  GPUBuffer *vertex_buffer;
  GPUBuffer *index_buffer;
};