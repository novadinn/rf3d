#include "opengl_buffer.h"

#include "logger.h"

#include <string.h>

bool OpenGLBuffer::Create(GPUBufferType buffer_type, uint64_t buffer_size) {
  internal_type = GL_NONE;

  switch (buffer_type) {
  case GPU_BUFFER_TYPE_VERTEX:
  case GPU_BUFFER_TYPE_INDEX:
  case GPU_BUFFER_TYPE_STAGING: {
    internal_type = GL_ARRAY_BUFFER;
  } break;
  case GPU_BUFFER_TYPE_UNIFORM: {
    internal_type = GL_UNIFORM_BUFFER;
  } break;
  default: {
    ERROR("Unsupported buffer type!");
    return false;
  }
  }

  type = buffer_type;
  total_size = buffer_size;

  glGenBuffers(1, &id);
  glBindBuffer(internal_type, id);
  /* TODO: add usage flags */
  glBufferData(internal_type, total_size, 0, GL_STATIC_DRAW);
  glBindBuffer(internal_type, 0);

  return true;
}

void OpenGLBuffer::Destroy() {
  glDeleteBuffers(1, &id);
  internal_type = GL_NONE;
  type = GPU_BUFFER_TYPE_NONE;
  total_size = 0;
  id = GL_NONE;
}

bool OpenGLBuffer::Bind(uint64_t offset) {
  glBindBuffer(internal_type, id);
  return true;
}

void *OpenGLBuffer::Lock(uint64_t offset, uint64_t size) {
  return glMapBufferRange(internal_type, offset, size, GL_MAP_WRITE_BIT);
}

void OpenGLBuffer::Unlock() { glUnmapBuffer(internal_type); }

bool OpenGLBuffer::LoadData(uint64_t offset, uint64_t size, void *data) {
  glBindBuffer(internal_type, id);
  glBufferSubData(internal_type, offset, size, data);
  glBindBuffer(internal_type, 0);

  return true;
}

bool OpenGLBuffer::CopyTo(GPUBuffer *dest, uint64_t source_offset,
                          uint64_t dest_offset, uint64_t size) {
  glCopyBufferSubData(internal_type, ((OpenGLBuffer *)dest)->GetInternalType(),
                      source_offset, dest_offset, size);
  return false;
}