#pragma once

#include "vulkan_attachment.h"
#include "vulkan_framebuffer.h"
#include "vulkan_render_pass.h"

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
  inline uint32_t GetMaxFramesInFlights() const { return max_frames_in_flight; }
  inline std::vector<GPUAttachment *> &GetColorAttachments() {
    return color_attachments;
  }
  inline VulkanAttachment &GetDepthAttachment() { return depth_attachment; }
  inline VkSurfaceFormatKHR GetImageFormat() const { return image_format; }
  inline VkExtent2D GetExtent() const { return extent; }

  inline uint32_t GetImageCount() const { return color_attachments.size(); }

private:
  VkSwapchainKHR handle;
  uint32_t max_frames_in_flight;
  std::vector<GPUAttachment *> color_attachments;
  VkSurfaceFormatKHR image_format;
  VkPresentModeKHR present_mode;
  VkExtent2D extent;
  VulkanAttachment depth_attachment;
};