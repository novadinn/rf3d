#include "opengl_utils.h"

#include "logger.h"

GLuint OpenGLUtils::GPUShaderStageTypeToOpenGLStage(GPUShaderStageType stage) {
  switch (stage) {
  case GPU_SHADER_STAGE_TYPE_VERTEX: {
    return GL_VERTEX_SHADER;
  } break;
  case GPU_SHADER_STAGE_TYPE_FRAGMENT: {
    return GL_FRAGMENT_SHADER;
  } break;
  default: {
    ERROR("Unsupported shader stage!");
    return GL_NONE;
  } break;
  }

  return GL_NONE;
}

GLuint OpenGLUtils::GPUFormatToOpenGLFormat(GPUFormat format) {
  switch (format) {
  case GPU_FORMAT_RG32F: {
    return GL_RG32F;
  } break;
  case GPU_FORMAT_RGB32F: {
    return GL_RGB32F;
  } break;
  case GPU_FORMAT_RGB8: {
    return GL_RG8;
  } break;
  case GPU_FORMAT_RGBA8: {
    return GL_RGBA8;
  } break;
  case GPU_FORMAT_D24_S8: {
    return GL_DEPTH24_STENCIL8;
  } break;
  default: {
    ERROR("Unsupported gpu format!");
    return GL_NONE;
  } break;
  }

  return GL_NONE;
}

GLuint OpenGLUtils::OpenGLInternalFormatToDataFormat(GLuint format) {
  switch (format) {
  case GL_RG8: {
    return GL_RG;
  } break;
  case GL_RGB8: {
    return GL_RGB;
  } break;
  case GL_RGBA8: {
    return GL_RGBA;
  } break;
  case GL_DEPTH24_STENCIL8: {
    return GL_DEPTH_STENCIL_ATTACHMENT;
  } break;
  default: {
    ERROR("Unsupported internal format!");
    return GL_NONE;
  } break;
  }

  return GL_NONE;
}