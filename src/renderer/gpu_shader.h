#pragma once

#include "gpu_buffer.h"
#include "gpu_core.h"
#include "gpu_render_pass.h"
#include "gpu_shader_buffer.h"

#include <list>
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

struct GPUShaderPushConstantConfig {
  uint8_t stage_flags;
  uint32_t offset;
  uint32_t size;
};

struct GPUShaderConfig {
  std::vector<GPUShaderStageConfig> stage_configs;
  std::vector<GPUShaderAttributeConfig> attribute_configs;
  std::vector<GPUShaderBuffer *> descriptors;
  std::vector<GPUShaderPushConstantConfig> push_constant_configs;
};

class GPUShader {
public:
  virtual ~GPUShader(){};

  virtual bool Create(GPUShaderConfig *config, GPURenderPass *render_pass,
                      float viewport_width, float viewport_height) = 0;
  virtual void Destroy() = 0;

  virtual void Bind() = 0;
  virtual void PushConstant(void *value, uint64_t size, uint32_t offset,
                            uint8_t stage_flags) = 0;

protected:
};