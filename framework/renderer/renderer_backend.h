#pragma once

#include "gpu_descriptor_set.h"
#include "gpu_index_buffer.h"
#include "gpu_render_pass.h"
#include "gpu_render_target.h"
#include "gpu_shader.h"
#include "gpu_uniform_buffer.h"
#include "gpu_vertex_buffer.h"

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
  virtual bool DrawIndexed(uint32_t element_count) = 0;

  virtual GPURenderPass *GetWindowRenderPass() = 0;
  virtual GPURenderTarget *GetCurrentWindowRenderTarget() = 0;
  virtual uint32_t GetCurrentFrameIndex() = 0;
  virtual uint32_t GetMaxFramesInFlight() = 0;

  virtual void BeginDebugRegion(const char *name, glm::vec4 color) = 0;
  virtual void InsertDebugMarker(const char *name, glm::vec4 color) = 0;
  virtual void EndDebugRegion() = 0;

  virtual GPUVertexBuffer *VertexBufferAllocate() = 0;
  virtual GPUIndexBuffer *IndexBufferAllocate() = 0;
  virtual GPUUniformBuffer *UniformBufferAllocate() = 0;
  virtual GPURenderPass *RenderPassAllocate() = 0;
  virtual GPURenderTarget *RenderTargetAllocate() = 0;
  virtual GPUShader *ShaderAllocate() = 0;
  virtual GPUTexture *TextureAllocate() = 0;
  virtual GPUAttachment *AttachmentAllocate() = 0;
  virtual GPUDescriptorSet *DescriptorSetAllocate() = 0;
};