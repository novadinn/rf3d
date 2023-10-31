#include "opengl_render_pass.h"

#include "opengl_framebuffer.h"

#include <glad/glad.h>

void OpenGLRenderPass::Create(
    std::vector<GPURenderPassAttachment> pass_render_attachments,
    glm::vec4 pass_render_area, glm::vec4 pass_clear_color, float pass_depth,
    float pass_stencil, uint8_t pass_clear_flags) {
  attachments = pass_render_attachments;
  render_area = pass_render_area;
  clear_color = pass_clear_color;
  clear_flags = pass_clear_flags;
}

void OpenGLRenderPass::Destroy() {
  attachments.clear();
  render_area = glm::vec4(0.0f);
  clear_color = glm::vec4(0.0f);
  clear_flags = 0;
}

void OpenGLRenderPass::Begin(GPURenderTarget *target) {
  GLuint framebuffer_id = 0;
  /* if target is not set, we render to the default framebuffer with an id of 0
   */
  if (target) {
    OpenGLFramebuffer *native_render_target = (OpenGLFramebuffer *)target;
    framebuffer_id = native_render_target->GetID();
  }
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
  glViewport(render_area.x, render_area.y, render_area.z, render_area.w);

  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

  GLbitfield native_clear_flags = 0;
  if (clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_COLOR) {
    native_clear_flags |= GL_COLOR_BUFFER_BIT;
  }
  if (clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_DEPTH) {
    native_clear_flags |= GL_DEPTH_BUFFER_BIT;
  }
  if (clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_STENCIL) {
    native_clear_flags |= GL_STENCIL_BUFFER_BIT;
  }

  glClear(native_clear_flags);
}

void OpenGLRenderPass::End() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }