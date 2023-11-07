#pragma once

#include "gpu_buffer.h"
#include "gpu_core.h"
#include "gpu_render_pass.h"
#include "gpu_texture.h"

#include <list>
#include <vector>

struct GPUShaderPushConstant {
  void *value;
  uint64_t size;
  uint32_t offset;
  uint8_t stage_flags;
};

enum GPUShaderStageType {
  GPU_SHADER_STAGE_TYPE_VERTEX,
  GPU_SHADER_STAGE_TYPE_FRAGMENT,
  /* TODO: others */
};

struct GPUShaderStageConfig {
  GPUShaderStageType type;
  const char *file_path;
};

struct GPUShaderConfig {
  std::vector<GPUShaderStageConfig> stage_configs;
};

class GPUShader {
public:
  virtual ~GPUShader(){};

  virtual bool Create(GPUShaderConfig *config, GPURenderPass *render_pass,
                      float viewport_width, float viewport_height) = 0;
  virtual void Destroy() = 0;

  virtual GPUBuffer *GetShaderBuffer(uint32_t set, uint32_t binding) = 0;

  virtual void Bind() = 0;
  virtual void BindShaderBuffer(uint32_t set, uint32_t binding) = 0;
  virtual void PushConstant(GPUShaderPushConstant *push_constant) = 0;
  virtual void SetTexture(uint32_t index, GPUTexture *texture) = 0;

protected:
};