#pragma once

#include "renderer/gpu_buffer.h"
#include "renderer/gpu_shader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

class OpenGLShader : public GPUShader {
public:
  OpenGLShader() = default;

  bool Create(GPUShaderConfig *config, GPURenderPass *render_pass,
              float viewport_width, float viewport_height) override;

  void Destroy() override;

  void Bind() override;

  inline GLuint GetID() { return id; }

private:
  GLuint id;
};