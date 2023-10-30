#pragma once

#include "renderer/gpu_buffer.h"
#include "renderer/gpu_shader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

struct OpenGLShaderDescriptor {
  GPUShaderDescriptorType type;
  GPUBuffer *buffer;
};

class OpenGLShader : public GPUShader {
public:
  OpenGLShader() = default;

  bool Create(GPUShaderConfig *config, GPURenderPass *render_pass,
              float viewport_width, float viewport_height) override;

  void Destroy() override;

  void Bind() override;

  GPUBuffer *GetDescriptorBuffer(const char *name) override;

private:
  std::unordered_map<const char *, OpenGLShaderDescriptor> descriptors;
  GLuint id;
};