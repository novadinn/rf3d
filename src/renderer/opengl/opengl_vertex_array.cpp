#include "opengl_vertex_array.h"

#include "logger.h"
#include "opengl_utils.h"
#include "renderer/gpu_utils.h"

void OpenGLVertexArray::Create(GPUBuffer *vertex_buffer,
                               GPUBuffer *index_buffer,
                               std::vector<GPUFormat> &attribute_formats) {
  glGenVertexArrays(1, &id);

  uint32_t total = 0;

  for (int i = 0; i < attribute_formats.size(); ++i) {
    total += GPUUtils::GetGPUFormatSize(attribute_formats[i]) *
             GPUUtils::GetGPUFormatCount(attribute_formats[i]);
  }

  Bind();
  vertex_buffer->Bind(0);

  uint32_t offset = 0;

  for (int i = 0; i < attribute_formats.size(); ++i) {
    GLuint native_format =
        OpenGLUtils::GPUFormatToOpenGLFormat(attribute_formats[i]);
    int count = GPUUtils::GetGPUFormatCount(attribute_formats[i]);
    int size = GPUUtils::GetGPUFormatSize(attribute_formats[i]);

    glVertexAttribPointer(i, count, GL_FLOAT, GL_FALSE, total,
                          (void *)(offset * size));
    glEnableVertexAttribArray(i);
    offset += count;
  }

  if (index_buffer != nullptr) {
    Bind();
    index_buffer->Bind(0);
  }
}

void OpenGLVertexArray::Destroy() {
  glDeleteVertexArrays(1, &id);
  id = GL_NONE;
}

void OpenGLVertexArray::Bind() { glBindVertexArray(id); }