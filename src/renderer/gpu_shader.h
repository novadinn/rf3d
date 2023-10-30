#pragma once

#include "gpu_buffer.h"
#include "gpu_core.h"
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

enum GPUShaderDescriptorType {
  GPU_SHADER_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
};

struct GPUShaderAttributeConfig {
  GPUFormat format;
};

struct GPUShaderDescriptorConfig {
  const char *name;
  GPUShaderDescriptorType type;
  uint8_t stage_flags;
  uint64_t size;
};

struct GPUShaderConfig {
  std::vector<GPUShaderStageConfig> stage_configs;
  std::vector<GPUShaderAttributeConfig> attribute_configs;
  std::vector<GPUShaderDescriptorConfig> descriptor_configs;
};

class GPUShader {
public:
  virtual ~GPUShader(){};

  virtual bool Create(GPUShaderConfig *config, GPURenderPass *render_pass,
                      float viewport_width, float viewport_height) = 0;
  virtual void Destroy() = 0;

  virtual void Bind() = 0;

  virtual GPUBuffer *GetDescriptorBuffer(const char *name) = 0;

protected:
};