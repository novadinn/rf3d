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

  return true;
}

void OpenGLShader::Destroy() {
  glDeleteProgram(id);
  id = GL_NONE;
}

void OpenGLShader::Bind() { glUseProgram(id); }

void OpenGLShader::PushConstant(void *value, uint64_t size, uint32_t offset,
                                uint8_t stage_flags) {
  /* TODO: do nothing - unsupported in opengl. We can add support via making a
   * pushconstant struct that holds a list of void *, but that will complicate a
   * code a lot, so dont care for now */
}