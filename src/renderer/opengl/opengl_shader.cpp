#include "opengl_shader.h"

#include "logger.h"
#include "opengl_backend.h"
#include "opengl_utils.h"

#include <cstdio>
#include <fstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

bool OpenGLShader::Create(GPUShaderConfig *config, GPURenderPass *render_pass,
                          float viewport_width, float viewport_height) {
  /* compile the shader */
  std::vector<GLuint> stages;
  for (int i = 0; i < config->stage_configs.size(); ++i) {
    GPUShaderStageConfig *stage_config = &config->stage_configs[i];

    std::string code;
    std::ifstream fp;

    fp.exceptions(std::ifstream::failbit);

    try {
      fp.open(stage_config->file_path);
      std::stringstream stream;

      stream << fp.rdbuf();

      fp.close();
      code = stream.str();
    } catch (std::ifstream::failure &e) {
      ERROR("%s", e.what());
      return false;
    }

    GLuint stage = glCreateShader(
        OpenGLUtils::GPUShaderStageTypeToOpenGLStage(stage_config->type));
    const GLchar *src = (const GLchar *)code.c_str();
    glShaderSource(stage, 1, &src, 0);
    glCompileShader(stage);

    GLint completed = 0;
    glGetShaderiv(stage, GL_COMPILE_STATUS, &completed);
    if (completed == GL_FALSE) {
      GLint log_length = 0;
      glGetShaderiv(stage, GL_INFO_LOG_LENGTH, &log_length);
      std::vector<GLchar> log;
      log.resize(log_length);
      glGetShaderInfoLog(stage, log_length, &log_length, log.data());
      glDeleteShader(stage);

      ERROR("Failed to load shader stage. Error code: %s", log.data());
      return false;
    }

    stages.emplace_back(stage);
  }

  id = glCreateProgram();
  for (int i = 0; i < stages.size(); ++i) {
    glAttachShader(id, stages[i]);
  }

  glLinkProgram(id);
  GLint link = 0;
  glGetProgramiv(id, GL_LINK_STATUS, &link);
  if (link == GL_FALSE) {
    GLint log_length = 0;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);

    std::vector<GLchar> log;
    log.resize(log_length);
    glGetProgramInfoLog(id, log_length, &log_length, log.data());

    glDeleteProgram(id);
    for (int i = 0; i < stages.size(); ++i) {
      glDeleteShader(stages[i]);
    }

    ERROR("Failed to create program. Error code: %s", log.data());
    return false;
  }

  for (int i = 0; i < stages.size(); ++i) {
    glDetachShader(id, stages[i]);
  }

  /* set uniform data */
  for (int i = 0; i < config->descriptor_configs.size(); ++i) {
    GPUShaderDescriptorConfig *descriptor_config =
        &config->descriptor_configs[i];

    OpenGLShaderDescriptor descriptor;
    descriptor.type = descriptor_config->type;
    /* TODO: only ubos are supported rn */

    glUniformBlockBinding(
        id, glGetUniformBlockIndex(id, descriptor_config->name), i);
    descriptor.buffer = new OpenGLBuffer();
    OpenGLBuffer *native_descriptor_buffer = (OpenGLBuffer *)descriptor.buffer;
    descriptor.buffer->Create(GPU_BUFFER_TYPE_UNIFORM, descriptor_config->size);

    descriptors.emplace(descriptor_config->name, descriptor);
  }

  return true;
}

void OpenGLShader::Destroy() {
  glDeleteProgram(id);
  id = GL_NONE;
  for (auto it = descriptors.begin(); it != descriptors.end(); ++it) {
    it->second.buffer->Destroy();
    delete it->second.buffer;
  }

  descriptors.clear();
}

void OpenGLShader::Bind() {
  glUseProgram(id);
  /* TODO: add descriptor index instead */
  int i = 0;
  for (auto it = descriptors.begin(); it != descriptors.end(); ++it) {
    OpenGLBuffer *native_descriptor_buffer = (OpenGLBuffer *)it->second.buffer;
    glBindBufferRange(native_descriptor_buffer->GetInternalType(), i,
                      native_descriptor_buffer->GetID(), 0,
                      native_descriptor_buffer->GetSize());
    ++i;
  }
}

GPUBuffer *OpenGLShader::GetDescriptorBuffer(const char *name) {
  return descriptors[name].buffer;
}