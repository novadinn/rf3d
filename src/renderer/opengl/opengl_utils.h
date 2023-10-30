#pragma once

#include "renderer/gpu_shader.h"

#include <glad/glad.h>

class OpenGLUtils {
public:
  static GLuint GPUShaderStageTypeToOpenGLStage(GPUShaderStageType stage);
  static GLuint GPUFormatToOpenGLFormat(GPUFormat format);
  static GLuint OpenGLInternalFormatToDataFormat(GLuint format);
};