#include "opengl_framebuffer.h"

#include "logger.h"
#include "opengl_utils.h"

bool OpenGLFramebuffer::Create(GPURenderPass *target_render_pass,
                               std::vector<GPUTexture *> target_attachments,
                               uint32_t target_width, uint32_t target_height) {
  attachments = target_attachments;
  width = target_width;
  height = target_height;

  return Invalidate();
}

void OpenGLFramebuffer::Destroy() {
  if (id != GL_NONE) {
    glDeleteFramebuffers(1, &id);
  }

  attachments.clear();
  width = 0;
  height = 0;
  id = GL_NONE;
}

bool OpenGLFramebuffer::Resize(uint32_t new_width, uint32_t new_height) {
  width = new_width;
  height = new_height;

  return Invalidate();
}

bool OpenGLFramebuffer::Invalidate() {
  if (id != GL_NONE) {
    glDeleteFramebuffers(1, &id);
  }

  /* TODO: no multisampling support for now */
  GLenum texture_target = GL_TEXTURE_2D;

  glGenFramebuffers(1, &id);
  glBindFramebuffer(GL_FRAMEBUFFER, id);

  uint32_t color_attachment_count = 0;
  for (int i = 0; i < attachments.size(); ++i) {
    GPUFormat temp_format = attachments[i]->GetFormat();
    GPUTextureAspect temp_aspect = attachments[i]->GetUsage();
    uint32_t temp_width = attachments[i]->GetWidth();
    uint32_t temp_height = attachments[i]->GetHeight();
    /* recreate textures, since they are bound at framebuffer's creation time */
    attachments[i]->Destroy();
    attachments[i]->Create(temp_format, temp_aspect, temp_width, temp_height);

    switch (attachments[i]->GetFormat()) {
    case GPU_FORMAT_RGBA8: {
      AttachColorTexture(GL_RGBA8, GL_RGBA, i);
      color_attachment_count++;
    } break;
    case GPU_FORMAT_D24_S8: {
      AttachDepthTexture(GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, i);
    } break;
    default: {
      ERROR("Unsupported framebuffer attachment format!");
      return false;
    } break;
    }
  }

  if (color_attachment_count > 1) {
    if (color_attachment_count > 4) {
      ERROR("Number of frambuffer color attachments exceeded maximum of %d", 4);
      Destroy();
      return false;
    }

    GLenum buffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                         GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(color_attachment_count, buffers);
  } else if (color_attachment_count == 0) {
    glDrawBuffer(GL_NONE);
  }

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    ERROR("Can not create framebuffer!");

    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return true;
}

void OpenGLFramebuffer::AttachColorTexture(GLenum internal_format,
                                           GLenum format, int index) {
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index,
                         GL_TEXTURE_2D,
                         ((OpenGLTexture *)attachments[index])->GetID(), 0);
}

void OpenGLFramebuffer::AttachDepthTexture(GLenum format,
                                           GLenum attachment_type, int index) {
  glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_type, GL_TEXTURE_2D,
                         ((OpenGLTexture *)attachments[index])->GetID(), 0);
}