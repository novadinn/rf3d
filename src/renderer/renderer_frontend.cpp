#include "renderer_frontend.h"

#include "logger.h"
#include "opengl/opengl_backend.h"
#include "vulkan/vulkan_backend.h"

bool RendererFrontend::Initialize(SDL_Window *window,
                                  RendererBackendType backend_type) {
  switch (backend_type) {
  case RendererBackendType::RBT_VULKAN: {
    backend = new VulkanBackend();
    return backend->Initialize(window);
  } break;
  case RendererBackendType::RBT_OPENGL: {
    backend = new OpenGLBackend();
    return backend->Initialize(window);
  }
  default: {
    ERROR("Renderer backend type is not supported!");
    return false;
  } break;
  }

  return false;
}

void RendererFrontend::Shutdown() {
  backend->Shutdown();

  delete backend;

  backend = 0;
}

void RendererFrontend::Resize(uint32_t width, uint32_t height) {
  backend->Resize(width, height);
}

bool RendererFrontend::BeginFrame() { return backend->BeginFrame(); }

bool RendererFrontend::EndFrame() { return backend->EndFrame(); }

bool RendererFrontend::Draw(uint32_t element_count) {
  return backend->Draw(element_count);
}

GPURenderPass *RendererFrontend::GetWindowRenderPass() {
  return backend->GetWindowRenderPass();
}

GPURenderTarget *RendererFrontend::GetCurrentWindowRenderTarget() {
  return backend->GetCurrentWindowRenderTarget();
}

GPUBuffer *RendererFrontend::BufferAllocate() {
  return backend->BufferAllocate();
}

GPURenderPass *RendererFrontend::RenderPassAllocate() {
  return backend->RenderPassAllocate();
}

GPURenderTarget *RendererFrontend::RenderTargetAllocate() {
  return backend->RenderTargetAllocate();
}

GPUShader *RendererFrontend::ShaderAllocate() {
  return backend->ShaderAllocate();
}

GPUAttributeArray *RendererFrontend::AttributeArrayAllocate() {
  return backend->AttributeArrayAllocate();
}

GPUTexture *RendererFrontend::TextureAllocate() {
  return backend->TextureAllocate();
}

GPUDescriptor *RendererFrontend::DescriptorAllocate() {
  return backend->DescriptorAllocate();
}