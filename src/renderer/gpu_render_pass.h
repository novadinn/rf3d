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

struct GPURenderPassAttachment {
  GPUFormat format;
  GPUTextureUsage usage;
};

/* there is no concept of a render pass in opengl, but we still need to do smth
 * with a framebuffers */
class GPURenderPass {
public:
  virtual ~GPURenderPass() {}

  virtual void
  Create(std::vector<GPURenderPassAttachment> pass_render_attachments,
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
  std::vector<GPURenderPassAttachment> attachments;
  glm::vec4 render_area;
  glm::vec4 clear_color;
  uint8_t clear_flags;
};