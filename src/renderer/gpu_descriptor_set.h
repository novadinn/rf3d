#pragma once

#include "gpu_attachment.h"
#include "gpu_texture.h"
#include "gpu_uniform_buffer.h"

#include <stdint.h>
#include <vector>

enum GPUDescriptorBindingType {
  GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER,
  GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE,
  GPU_DESCRIPTOR_BINDING_TYPE_ATTACHMENT,
};

struct GPUDescriptorBinding {
  uint32_t binding;
  GPUDescriptorBindingType type;
  GPUTexture *texture;
  GPUUniformBuffer *uniform_buffer;
  GPUAttachment *attachment;
};

class GPUDescriptorSet {
public:
  virtual ~GPUDescriptorSet() {}
  virtual void Create(uint32_t set_index,
                      std::vector<GPUDescriptorBinding> &set_bindings) = 0;
  virtual void Destroy() = 0;

  inline uint32_t GetIndex() const { return index; }
  inline std::vector<GPUDescriptorBinding> &GetBindings() { return bindings; }

protected:
  uint32_t index;
  std::vector<GPUDescriptorBinding> bindings;
};