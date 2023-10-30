#pragma once

#include "gpu_buffer.h"
#include "gpu_core.h"

#include <vector>

/* there is not concept of vao's in vulkan, but it is still better to have it
 * for opengl - we dont need to create a vao every time when we create an opengl
 * shader */
class GPUAttributeArray {
public:
  virtual ~GPUAttributeArray(){};

  virtual void Create(GPUBuffer *vertex_buffer, GPUBuffer *index_buffer,
                      std::vector<GPUFormat> &attribute_formats) = 0;
  virtual void Destroy() = 0;

  virtual void Bind() = 0;

protected:
};