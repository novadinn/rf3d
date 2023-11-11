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