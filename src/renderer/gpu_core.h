#pragma once

#include <stdint.h>
#include <unordered_map>

enum GPUFormat {
  GPU_FORMAT_NONE,
  GPU_FORMAT_RG32F,
  GPU_FORMAT_RGB32F,
  GPU_FORMAT_RGB8,
  GPU_FORMAT_RGBA8,
  GPU_FORMAT_D24_S8,
  /* TODO: others */
};

struct GPUShaderBufferIndex {
  uint32_t set, binding;

  bool operator==(const GPUShaderBufferIndex &other) const {
    return (set == other.set && binding == other.binding);
  }
};

template <> struct std::hash<GPUShaderBufferIndex> {
  std::size_t operator()(const GPUShaderBufferIndex &index) const {
    return ((std::hash<uint32_t>()(index.set) ^
             (std::hash<uint32_t>()(index.binding) << 1)) >>
            1);
  }
};