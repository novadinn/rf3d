#pragma once

#include "gpu_buffer.h"
#include "gpu_core.h"
#include "gpu_descriptor.h"
#include "gpu_render_pass.h"

#include <vector>

enum GPUShaderStageType {
  GPU_SHADER_STAGE_TYPE_VERTEX,
  GPU_SHADER_STAGE_TYPE_FRAGMENT,
  /* TODO: others */
};

struct GPUShaderStageConfig {
  GPUShaderStageType type;
  const char *file_path;
};

struct GPUShaderAttributeConfig {
  GPUFormat format;
};

struct GPUShaderConfig {
  std::vector<GPUShaderStageConfig> stage_configs;
  std::vector<GPUShaderAttributeConfig> attribute_configs;
  std::vector<GPUDescriptor *> descriptors;
};

class GPUShader {
public:
  virtual ~GPUShader(){};

  virtual bool Create(GPUShaderConfig *config, GPURenderPass *render_pass,
                      float viewport_width, float viewport_height) = 0;
  virtual void Destroy() = 0;

  virtual void Bind() = 0;

protected:
};