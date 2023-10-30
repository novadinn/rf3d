#pragma once

#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"

#include <vulkan/vulkan.h>

class VulkanImage {
public:
  void Create(int image_width, int image_height, VkFormat format,
              VkImageTiling tiling, VkImageUsageFlags usage_flags,
              VkMemoryPropertyFlags memory_flags,
              VkImageAspectFlags aspect_flags);
  void Destroy();

  void CreateImageView(VkFormat format, VkImageAspectFlags aspect_flags);

  void TransitionLayout(VulkanCommandBuffer *command_buffer, VkFormat format,
                        VkImageLayout old_layout, VkImageLayout new_layout);
  void CopyFromBuffer(VulkanBuffer *buffer,
                      VulkanCommandBuffer *command_buffer);

  void SetImageView(VkImageView new_view) { view = new_view; }

  inline VkImageView GetImageView() const { return view; }
  inline int GetWidth() const { return width; }
  inline int GetHeight() const { return height; }

private:
  VkImage handle;
  VkImageView view;
  VkDeviceMemory memory;

  int width;
  int height;
};