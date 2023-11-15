#pragma once

#include "gpu_core.h"
#include "gpu_descriptor_set.h"
#include "gpu_render_pass.h"
#include "gpu_texture.h"
#include "gpu_uniform_buffer.h"

#include <vector>

enum GPUShaderStageType {
  GPU_SHADER_STAGE_TYPE_VERTEX,
  GPU_SHADER_STAGE_TYPE_FRAGMENT,
};

enum GPUShaderTopologyType {
  GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
  GPU_SHADER_TOPOLOGY_TYPE_LINE_LIST,
};

struct GPUShaderStageConfig {
  GPUShaderStageType type;
  const char *file_path;
};

class GPUShader {
public:
  virtual ~GPUShader(){};

  virtual bool Create(std::vector<GPUShaderStageConfig> stage_configs,
                      GPUShaderTopologyType topology_type,
                      GPURenderPass *render_pass, float viewport_width,
                      float viewport_height) = 0;
  virtual void Destroy() = 0;

  virtual void Bind() = 0;
  virtual void BindUniformBuffer(GPUDescriptorSet *set, uint32_t offset) = 0;
  virtual void BindTexture(GPUDescriptorSet *set) = 0;
  virtual void PushConstant(void *value, uint64_t size, uint32_t offset,
                            uint8_t stage_flags) = 0;

protected:
};