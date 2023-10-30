#pragma once

#include "renderer/renderer_backend.h"
#include "vulkan_context.h"

#include "vulkan_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_render_pass.h"
#include "vulkan_shader.h"

class VulkanBackend : public RendererBackend {
public:
  bool Initialize(SDL_Window *sdl_window) override;
  void Shutdown() override;

  void Resize(uint32_t width, uint32_t height) override;

  bool BeginFrame() override;
  bool EndFrame() override;
  bool Draw(uint32_t element_count) override;

  GPURenderPass *GetWindowRenderPass() override;
  GPURenderTarget *GetCurrentWindowRenderTarget() override;

  GPUBuffer *BufferAllocate() override;
  GPURenderTarget *RenderTargetAllocate() override;
  GPURenderPass *RenderPassAllocate() override;
  GPUShader *ShaderAllocate() override;
  GPUAttributeArray *AttributeArrayAllocate() override;
  GPUTexture *TextureAllocate() override;

  static VulkanContext *GetContext();

private:
  static VulkanContext *context;
  SDL_Window *window;

  GPURenderPass *main_render_pass;

  /* TODO: temp */
  float width, height;
};