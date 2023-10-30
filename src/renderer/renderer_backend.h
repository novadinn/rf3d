#pragma once

#include "renderer/gpu_attribute_array.h"
#include "renderer/gpu_buffer.h"
#include "renderer/gpu_render_pass.h"
#include "renderer/gpu_render_target.h"
#include "renderer/gpu_shader.h"

#include <stdint.h>

struct SDL_Window;

class RendererBackend {
public:
  virtual ~RendererBackend(){};
  virtual bool Initialize(SDL_Window *window) = 0;
  virtual void Shutdown() = 0;

  virtual void Resize(uint32_t width, uint32_t height) = 0;

  virtual bool BeginFrame() = 0;
  virtual bool EndFrame() = 0;
  virtual bool Draw(uint32_t element_count) = 0;

  virtual GPURenderPass *GetWindowRenderPass() = 0;
  virtual GPURenderTarget *GetCurrentWindowRenderTarget() = 0;

  virtual GPUBuffer *BufferAllocate() = 0;
  virtual GPURenderPass *RenderPassAllocate() = 0;
  virtual GPURenderTarget *RenderTargetAllocate() = 0;
  virtual GPUShader *ShaderAllocate() = 0;
  virtual GPUAttributeArray *AttributeArrayAllocate() = 0;
  virtual GPUTexture *TextureAllocate() = 0;
};