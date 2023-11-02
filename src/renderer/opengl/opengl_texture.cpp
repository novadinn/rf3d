#include "opengl_texture.h"

#include "logger.h"
#include "opengl_utils.h"

void OpenGLTexture::Create(GPUFormat texture_format,
                           GPUTextureType texture_type, uint32_t texture_width,
                           uint32_t texture_height) {
  format = texture_format;
  type = texture_type;
  width = texture_width;
  height = texture_height;

  glGenTextures(1, &id);

  GLuint texture_target = GL_NONE;
  switch (texture_type) {
  case GPU_TEXTURE_TYPE_2D: {
    texture_target = GL_TEXTURE_2D;
  } break;
  default: {
    ERROR("Unsupported texture type!");
  } break;
  }

  glBindTexture(texture_target, id);

  GLenum native_format = OpenGLUtils::GPUFormatToOpenGLFormat(format);
  switch (type) {
  case GPU_TEXTURE_TYPE_2D: {
    glTexImage2D(texture_target, 0,
                 OpenGLUtils::OpenGLInternalFormatToDataFormat(native_format),
                 width, height, 0,
                 OpenGLUtils::OpenGLInternalFormatToDataFormat(native_format),
                 GL_UNSIGNED_BYTE, 0);
  } break;
  default: {
    ERROR("Unsupported texture type!");
  } break;
  }

  glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(texture_target, GL_TEXTURE_WRAP_R, GL_REPEAT);
}

void OpenGLTexture::Destroy() {
  glDeleteTextures(1, &id);
  id = GL_NONE;
  format = GPU_FORMAT_NONE;
  type = GPU_TEXTURE_TYPE_NONE;
  width = 0;
  height = 0;
}

void OpenGLTexture::WriteData(uint8_t *pixels, uint32_t offset) {
  glBindTexture(GL_TEXTURE_2D, id);

  GLuint internal_format = OpenGLUtils::GPUFormatToOpenGLFormat(format);
  GLuint data_format =
      OpenGLUtils::OpenGLInternalFormatToDataFormat(internal_format);
  glTexImage2D(GL_TEXTURE_2D, 0, data_format, width, height, 0, data_format,
               GL_UNSIGNED_BYTE, pixels);
  glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
}