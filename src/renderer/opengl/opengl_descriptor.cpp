#include "opengl_descriptor.h"

#include "opengl_buffer.h"
#include "opengl_shader.h"

void OpenGLDescriptor::Create(const char *descriptor_name,
                              GPUDescriptorType descriptor_type,
                              uint8_t descriptor_stage_flags,
                              uint64_t descriptor_size,
                              uint32_t descriptor_index) {
  /* TODO: only ubos are supported rn */
  name = descriptor_name;
  type = descriptor_type;
  index = descriptor_index;
  buffer = new OpenGLBuffer();
  buffer->Create(GPU_BUFFER_TYPE_UNIFORM, descriptor_size);
}

void OpenGLDescriptor::Destroy() {
  buffer->Destroy();
  delete buffer;
  name = "";
  type = GPU_DESCRIPTOR_TYPE_NONE;
}

void OpenGLDescriptor::Bind(GPUShader *shader) {
  GLuint id = ((OpenGLShader *)shader)->GetID();
  glUniformBlockBinding(id, glGetUniformBlockIndex(id, name), index);
  OpenGLBuffer *native_descriptor_buffer = (OpenGLBuffer *)buffer;
  glBindBufferRange(native_descriptor_buffer->GetInternalType(), index,
                    native_descriptor_buffer->GetID(), 0,
                    native_descriptor_buffer->GetSize());
}

GPUBuffer *OpenGLDescriptor::GetBuffer() { return buffer; }