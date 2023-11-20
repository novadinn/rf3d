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
  bool DrawIndexed(uint32_t element_count);

  GPURenderPass *GetWindowRenderPass();
  GPURenderTarget *GetCurrentWindowRenderTarget();
  /* TODO: those 2 are unused right now, but if we want a more smooth rendering,
   * all of the writable resources should be arrays and use those 2 methodss */
  uint32_t GetCurrentFrameIndex();
  uint32_t GetMaxFramesInFlight();

  GPUVertexBuffer *VertexBufferAllocate();
  GPUIndexBuffer *IndexBufferAllocate();
  GPUUniformBuffer *UniformBufferAllocate();
  GPURenderPass *RenderPassAllocate();
  GPURenderTarget *RenderTargetAllocate();
  GPUShader *ShaderAllocate();
  GPUTexture *TextureAllocate();
  GPUAttachment *AttachmentAllocate();
  GPUDescriptorSet *DescriptorSetAllocate();

private:
  RendererBackend *backend;
};