#pragma once

#include "gpu_core.h"

class GPUUtils {
public:
  static int GetGPUFormatSize(GPUFormat format);
  static int GetGPUFormatCount(GPUFormat format);
  static bool IsDepthFormat(GPUFormat format);
};