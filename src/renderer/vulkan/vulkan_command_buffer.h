#pragma once

#include <vulkan/vulkan.h>

class VulkanCommandBuffer {
public:
  void Allocate(VkCommandPool command_pool, VkCommandBufferLevel level);
  void Free(VkCommandPool command_pool);

  void Begin(VkCommandBufferUsageFlags usage);
  void End();

  void AllocateAndBeginSingleUse(VkCommandPool command_pool);
  void FreeAndEndSingleUse(VkCommandPool command_pool, VkQueue queue);

  void Reset(VkCommandBufferResetFlags flags);

  inline VkCommandBuffer &GetHandle() { return handle; }

private:
  VkCommandBuffer handle;
};