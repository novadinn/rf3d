#pragma once

#include <vulkan/vulkan.h>

class VulkanContext;

class VulkanFence {
public:
  void Create(bool create_signaled);
  void Destroy();

  bool Wait(uint64_t timeout_ns);
  void Reset();

  inline VkFence &GetHandle() { return handle; }

private:
  VkFence handle;
  bool signaled;
};