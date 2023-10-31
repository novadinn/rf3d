#pragma once

#include "vulkan_framebuffer.h"
#include "vulkan_render_pass.h"
#include "vulkan_texture.h"

#include <vector>
#include <vulkan/vulkan.h>

class VulkanContext;

class VulkanSwapchain {
public:
  bool Create(uint32_t width, uint32_t height);
  void Destroy();

  bool Recreate(uint32_t width, uint32_t height);
  bool AcquireNextImage(uint64_t timeout_ns, VkSemaphore semaphor,
                        VkFence fence, uint32_t width, uint32_t height,
                        uint32_t *out_image_index);

  inline VkSwapchainKHR &GetHandle() { return handle; }
  inline int GetMaxFramesInFlights() const { return max_frames_in_flight; }
  inline std::vector<VkImageView> &GetImageViews() { return image_views; }
  inline VulkanTexture &GetDepthImage() { return depth_attachment; }
  inline VkSurfaceFormatKHR GetImageFormat() const { return image_format; }
  inline VkExtent2D GetExtent() const { return extent; }

  inline int GetImagesCount() const { return images.size(); }

private:
  VkSwapchainKHR handle;
  int max_frames_in_flight;
  std::vector<VkImage> images;
  std::vector<VkImageView> image_views;
  VkSurfaceFormatKHR image_format;
  VkPresentModeKHR present_mode;
  VkExtent2D extent;
  VulkanTexture depth_attachment;
};