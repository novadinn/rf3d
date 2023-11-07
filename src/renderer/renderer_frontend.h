#pragma once

#include "renderer_backend.h"

struct SDL_Window;

enum RendererBackendType {
  RBT_VULKAN,
  RBT_METAL,
  RBT_DIRECTX,
};

class RendererFrontend {
public:
  bool Initialize(SDL_Window *window, RendererBackendType backend_type);
  void Shutdown();

  void Resize(uint32_t width, uint32_t height);

  bool BeginFrame();
  bool EndFrame();
  bool Draw(uint32_t element_count);

  GPURenderPass *GetWindowRenderPass();
  GPURenderTarget *GetCurrentWindowRenderTarget();

  GPUBuffer *BufferAllocate();
  GPURenderPass *RenderPassAllocate();
  GPURenderTarget *RenderTargetAllocate();
  GPUShader *ShaderAllocate();
  GPUTexture *TextureAllocate();

private:
  RendererBackend *backend;
};