#include "opengl_texture.h"

#include "logger.h"
#include "opengl_utils.h"

void OpenGLTexture::Create(GPUFormat image_format,
                           GPUTextureAspect texture_usage,
                           uint32_t texture_width, uint32_t texture_height) {
  format = image_format;
  aspect = texture_usage;
  width = texture_width;
  height = texture_height;

  glGenTextures(1, &id);

  glBindTexture(GL_TEXTURE_2D, id);

  GLenum native_format = OpenGLUtils::GPUFormatToOpenGLFormat(format);
  switch (texture_usage) {
  case GPU_TEXTURE_USAGE_COLOR_ATTACHMENT: {
    glTexImage2D(GL_TEXTURE_2D, 0, native_format, width, height, 0,
                 OpenGLUtils::OpenGLInternalFormatToDataFormat(native_format),
                 GL_UNSIGNED_BYTE, 0);
  } break;
  case GPU_TEXTURE_USAGE_DEPTH_ATTACHMENT:
  case GPU_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT: {
    glTexStorage2D(GL_TEXTURE_2D, 1, native_format, width, height);
  } break;
  default: {
    /* do nothing */
  } break;
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
}

void OpenGLTexture::Destroy() {
  glDeleteTextures(1, &id);
  id = GL_NONE;
  format = GPU_FORMAT_NONE;
  aspect = GPU_TEXTURE_USAGE_NONE;
  width = 0;
  height = 0;
}

void OpenGLTexture::WriteData(uint8_t *pixels, uint32_t offset) {
  glBindTexture(GL_TEXTURE_2D, id);

  GLuint internal_format = OpenGLUtils::GPUFormatToOpenGLFormat(format);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
               OpenGLUtils::OpenGLInternalFormatToDataFormat(internal_format),
               GL_UNSIGNED_BYTE, pixels);

  glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTexture::Resize(uint32_t new_width, uint32_t new_height) {
  Destroy();
  Create(format, aspect, new_width, new_height);
}