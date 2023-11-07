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
  GPUTexture *TextureAllocate() override;

  static VulkanContext *GetContext();

private:
  bool CreateInstance(VkApplicationInfo application_info, SDL_Window *window,
                      VkInstance *out_instance);
  bool CreateSurface(SDL_Window *window, VkSurfaceKHR *out_surface);
  bool CreateDebugMessanger(VkDebugUtilsMessengerEXT *out_debug_messenger);
  bool RequiredLayersAvailable(std::vector<const char *> required_layers);
  bool
  RequiredExtensionsAvailable(std::vector<const char *> required_extensions);

  void RegenerateFramebuffers();

  static VulkanContext *context;
  SDL_Window *window;

  GPURenderPass *main_render_pass;
  std::vector<GPURenderTarget *> main_framebuffers;
};