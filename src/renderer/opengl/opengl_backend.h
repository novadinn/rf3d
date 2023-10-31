#pragma once

#include "opengl_buffer.h"
#include "opengl_framebuffer.h"
#include "opengl_shader.h"
#include "opengl_vertex_array.h"
#include "renderer/renderer_backend.h"

#include <SDL2/SDL.h>

class OpenGLBackend : public RendererBackend {
public:
  bool Initialize(SDL_Window *sdl_window) override;
  void Shutdown() override;

  void Resize(uint32_t width, uint32_t height) override;

  bool BeginFrame() override;
  bool EndFrame() override;
  bool Draw(uint32_t element_count) override;

  GPURenderPass *GetWindowRenderPass() override;
  /* TODO: since opengl does not require you to create a default swapchain and
   * swapchain framebuffers, this will always return 0. So use another approach,
   * like BindMainRenderPass or smth */
  GPURenderTarget *GetCurrentWindowRenderTarget() override;

  GPUBuffer *BufferAllocate() override;
  GPURenderPass *RenderPassAllocate() override;
  GPURenderTarget *RenderTargetAllocate() override;
  GPUShader *ShaderAllocate() override;
  GPUAttributeArray *AttributeArrayAllocate() override;
  GPUTexture *TextureAllocate() override;

private:
  SDL_Window *window;
  SDL_GLContext gl_context;

  GPURenderPass *main_render_pass;
};