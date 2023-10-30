#include "gpu_utils.h"

#include "logger.h"

#include <stdint.h>

int GPUUtils::GetGPUFormatSize(GPUFormat format) {
  switch (format) {
  case GPU_FORMAT_RG32F: {
    return sizeof(float);
  } break;
  case GPU_FORMAT_RGB32F: {
    return sizeof(float);
  } break;
  case GPU_FORMAT_RGB8: {
    return sizeof(uint8_t);
  } break;
  case GPU_FORMAT_RGBA8: {
    return sizeof(uint8_t);
  } break;
  default: {
    ERROR("Failed to get gpu format size!");
    return 0;
  } break;
  }

  return 0;
}

int GPUUtils::GetGPUFormatCount(GPUFormat format) {
  switch (format) {
  case GPU_FORMAT_RG32F: {
    return 2;
  } break;
  case GPU_FORMAT_RGB32F: {
    return 3;
  } break;
  case GPU_FORMAT_RGB8: {
    return 2;
  } break;
  case GPU_FORMAT_RGBA8: {
    return 3;
  } break;
  case GPU_FORMAT_D24_S8: {
    return 2;
  } break;
  default: {
    ERROR("Failed to get gpu format count!");
    return 0;
  } break;
  }

  return 0;
}

bool GPUUtils::IsDepthFormat(GPUFormat format) {
  return format == GPU_FORMAT_D24_S8;
}