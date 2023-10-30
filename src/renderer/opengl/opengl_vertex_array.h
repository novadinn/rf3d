#pragma once

#include "opengl_buffer.h"
#include "renderer/gpu_attribute_array.h"
#include "renderer/gpu_shader.h"

#include <glad/glad.h>
#include <vector>

class OpenGLVertexArray : public GPUAttributeArray {
public:
  void Create(GPUBuffer *vertex_buffer, GPUBuffer *index_buffer,
              std::vector<GPUFormat> &attribute_formats) override;
  void Destroy() override;

  void Bind() override;

  inline GLuint GetID() const { return id; }

private:
  GLuint id;
};