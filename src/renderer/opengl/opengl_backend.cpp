#include "opengl_backend.h"

#include "logger.h"
#include "opengl_render_pass.h"
#include "opengl_shader_buffer.h"
#include "opengl_texture.h"
#include "platform.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

bool OpenGLBackend::Initialize(SDL_Window *sdl_window) {
  SDL_GL_LoadLibrary(NULL);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  /* Macos only supports 4.1 version and lower */
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef PLATFORM_APPLE
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                      SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

  window = sdl_window;
  gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
    ERROR("%s", SDL_GetError());
    return false;
  }

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    FATAL("Failed to initialize GLAD");
    return false;
  }

  SDL_GL_SetSwapInterval(1);
  SDL_GL_MakeCurrent(window, gl_context);

  glEnable(GL_DEPTH_TEST);

  int width, height;
  SDL_GetWindowSize(window, &width, &height);

  main_render_pass = RenderPassAllocate();
  main_render_pass->Create(
      std::vector<GPURenderPassAttachment>{}, glm::vec4(0, 0, width, height),
      glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), 1.0f, 0.0f,
      GPU_RENDER_PASS_CLEAR_FLAG_COLOR | GPU_RENDER_PASS_CLEAR_FLAG_DEPTH |
          GPU_RENDER_PASS_CLEAR_FLAG_STENCIL);

  return true;
}

void OpenGLBackend::Shutdown() {
  main_render_pass->Destroy();
  delete main_render_pass;
  window = 0;
  gl_context = 0;
  SDL_GL_DeleteContext(gl_context);
}

void OpenGLBackend::Resize(uint32_t width, uint32_t height) {}

bool OpenGLBackend::BeginFrame() {
  SDL_GL_MakeCurrent(window, gl_context);
  glm::vec4 &render_area = main_render_pass->GetRenderArea();
  glViewport(render_area.x, render_area.y, render_area.z, render_area.w);

  return true;
}

bool OpenGLBackend::EndFrame() {
  SDL_GL_SwapWindow(window);
  return true;
}

bool OpenGLBackend::Draw(uint32_t element_count) {
  /* TODO: index buffer support */
  glDrawArrays(GL_TRIANGLES, 0, element_count);

  return true;
}

GPURenderPass *OpenGLBackend::GetWindowRenderPass() { return main_render_pass; }

GPURenderTarget *OpenGLBackend::GetCurrentWindowRenderTarget() { return 0; }

GPUBuffer *OpenGLBackend::BufferAllocate() { return new OpenGLBuffer(); }

GPURenderTarget *OpenGLBackend::RenderTargetAllocate() {
  return new OpenGLFramebuffer();
}

GPURenderPass *OpenGLBackend::RenderPassAllocate() {
  return new OpenGLRenderPass();
}

GPUShader *OpenGLBackend::ShaderAllocate() { return new OpenGLShader(); }

GPUAttributeArray *OpenGLBackend::AttributeArrayAllocate() {
  return new OpenGLVertexArray();
}

GPUTexture *OpenGLBackend::TextureAllocate() { return new OpenGLTexture(); }

GPUShaderBuffer *OpenGLBackend::ShaderBufferAllocate() {
  return new OpenGLShaderBuffer();
}