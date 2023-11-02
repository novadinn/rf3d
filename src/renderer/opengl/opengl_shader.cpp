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

void OpenGLShader::PushConstant(GPUShaderPushConstant *push_constant) {
  uint32_t attribute_offset = 0;
  for (int i = 0; i < push_constant->values.size(); ++i) {
    std::string push_constant_name = std::string(push_constant->name);
    std::string value_name = std::string(push_constant->values[i].name);
    GPUShaderGLSLValueType value_type = push_constant->values[i].type;

    std::string attribute_name = push_constant_name + "." + value_name;
    void *target_ptr = ((char *)push_constant->value) + attribute_offset;

    switch (value_type) {
    case GPU_SHADER_GLSL_VALUE_TYPE_FLOAT: {
      float v = ((float *)target_ptr)[0];

      glUniform1f(glGetUniformLocation(id, attribute_name.c_str()), v);

      attribute_offset += 4;
    } break;
    default: {
      ERROR("Unsupported glsl value type!");
      return;
    } break;
    }
  }
}

void OpenGLShader::SetTexture(uint32_t index, GPUTexture *texture) {
  glActiveTexture(GL_TEXTURE0 + index);
  glBindTexture(GL_TEXTURE_2D, ((OpenGLTexture *)texture)->GetID());
}