#pragma once

#include "gpu_core.h"
#include "gpu_descriptor_set.h"
#include "gpu_render_pass.h"
#include "gpu_texture.h"
#include "gpu_uniform_buffer.h"

#include <stdint.h>
#include <stdio.h>
#include <vector>

enum GPUShaderStageType {
  GPU_SHADER_STAGE_TYPE_VERTEX,
  GPU_SHADER_STAGE_TYPE_FRAGMENT,
  GPU_SHADER_STAGE_TYPE_GEOMETRY,
  GPU_SHADER_STAGE_TYPE_TESSELLATION_CONTROL,
  GPU_SHADER_STAGE_TYPE_TESSELLATION_EVALUATION,
};

enum GPUShaderTopologyType {
  GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
  GPU_SHADER_TOPOLOGY_TYPE_LINE_LIST,
  /* required for shaders that contains tesselation stage */
  GPU_SHADER_TOPOLOGY_TYPE_PATCH_LIST,
};

enum GPUShaderDepthFlag {
  GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE = (1 << 0),
  GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE = (2 << 0),
};

enum GPUShaderStencilFlag {
  GPU_SHADER_STENCIL_FLAG_STENCIL_TEST_ENABLE = (1 << 0),
};

struct GPUShaderStageConfig {
  GPUShaderStageType type;
  const char *file_path;
};

struct GPUShaderConfig {
  std::vector<GPUShaderStageConfig> stage_configs;
  GPUShaderTopologyType topology_type; 
  uint8_t depth_flags;
  uint8_t stencil_flags;
  GPURenderPass *render_pass; 
  float viewport_width;
  float viewport_height;
};

class GPUShader {
public:
  virtual ~GPUShader(){};

  virtual bool Create(std::vector<GPUShaderStageConfig> stage_configs,
                      GPUShaderTopologyType topology_type, uint8_t depth_flags,
                      GPURenderPass *render_pass, float viewport_width,
                      float viewport_height) = 0;
  virtual void Destroy() = 0;

  virtual void Bind() = 0;
  virtual void BindUniformBuffer(GPUDescriptorSet *set, uint32_t offset,
                                 int32_t set_index) = 0;
  virtual void BindSampler(GPUDescriptorSet *set, int32_t set_index) = 0;
  virtual void PushConstant(void *value, uint64_t size, uint32_t offset,
                            uint8_t stage_flags) = 0;

  virtual void SetDebugName(const char *name) = 0;
  virtual void SetDebugTag(const void *tag, size_t tag_size) = 0;

protected:
};