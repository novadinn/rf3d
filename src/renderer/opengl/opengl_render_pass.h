#pragma once

#include "renderer/gpu_render_pass.h"

class OpenGLRenderPass : public GPURenderPass {
public:
  void Create(std::vector<GPURenderTarget *> pass_render_targets,
              glm::vec4 pass_render_area, glm::vec4 pass_clear_color,
              float pass_depth, float pass_stencil,
              uint8_t pass_clear_flags) override;
  void Destroy() override;

  void Begin(GPURenderTarget *target) override;
  void End() override;

private:
};