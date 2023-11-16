#pragma once

#include "gpu_core.h"
#include "gpu_render_target.h"

#include <glm/glm.hpp>
#include <stdint.h>

enum GPURenderPassClearFlag {
  GPU_RENDER_PASS_CLEAR_FLAG_NONE = 0x0,
  GPU_RENDER_PASS_CLEAR_FLAG_COLOR = 0x1,
  GPU_RENDER_PASS_CLEAR_FLAG_DEPTH = 0x2,
  GPU_RENDER_PASS_CLEAR_FLAG_STENCIL = 0x4,
};

enum GPURenderPassAttachmentLoadOperation {
  GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_DONT_CARE,
  GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_LOAD,
};

enum GPURenderPassAttachmentStoreOperation {
  GPU_RENDER_PASS_ATTACHMENT_STORE_OPERATION_DONT_CARE,
  GPU_RENDER_PASS_ATTACHMENT_STORE_OPERATION_STORE,
};

struct GPURenderPassAttachmentConfig {
  GPUFormat format;
  GPUAttachmentUsage usage;
  GPURenderPassAttachmentLoadOperation load_operation;
  GPURenderPassAttachmentStoreOperation store_operation;
  bool present_after;
};

class GPURenderPass {
public:
  virtual ~GPURenderPass() {}

  virtual bool
  Create(std::vector<GPURenderPassAttachmentConfig> pass_render_attachments,
         glm::vec4 pass_render_area, glm::vec4 pass_clear_color,
         float pass_depth, float pass_stencil, uint8_t pass_clear_flags) = 0;
  virtual void Destroy() = 0;

  virtual void Begin(GPURenderTarget *target) = 0;
  virtual void End() = 0;

  inline glm::vec4 &GetRenderArea() { return render_area; }
  inline void SetRenderArea(glm::vec4 new_render_area) {
    render_area = new_render_area;
  }

  inline glm::vec4 &GetClearColor() { return clear_color; }
  inline void SetClearColor(glm::vec4 new_clear_color) {
    clear_color = new_clear_color;
  }

protected:
  std::vector<GPURenderPassAttachmentConfig> attachments;
  glm::vec4 render_area;
  glm::vec4 clear_color;
  uint8_t clear_flags;
};