#pragma once

#include "opengl_texture.h"
#include "renderer/gpu_render_target.h"
#include "renderer/gpu_texture.h"

#include <glad/glad.h>
#include <vector>

class OpenGLFramebuffer : public GPURenderTarget {
public:
  bool Create(GPURenderPass *target_render_pass,
              std::vector<GPUTexture *> target_attachments,
              uint32_t target_width, uint32_t target_height) override;
  void Destroy() override;

  bool Resize(uint32_t new_width, uint32_t new_height) override;

  inline GLuint GetID() const { return id; }

private:
  bool Invalidate();

  void AttachColorTexture(GLenum internal_format, GLenum format, int index);
  void AttachDepthTexture(GLenum format, GLenum attachment_type, int index);

  GLuint id = GL_NONE;
};