#pragma once

#include "vulkan_framebuffer.h"
#include "vulkan_image.h"
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
  void RegenerateFramebuffers(VulkanRenderPass *render_pass, uint32_t width,
                              uint32_t height);

  inline VkSwapchainKHR &GetHandle() { return handle; }
  inline VkSurfaceFormatKHR GetImageFormat() const { return image_format; }
  inline int GetMaxFramesInFlights() const { return max_frames_in_flight; }
  inline int GetImagesCount() const { return images.size(); }
  inline VkExtent2D GetExtent() const { return extent; }

  inline std::vector<GPURenderTarget *> &GetFramebuffers() {
    return framebuffers;
  }

private:
  VkSwapchainKHR handle;
  int max_frames_in_flight;
  std::vector<VkImage> images;
  std::vector<VkImageView> image_views;
  VkSurfaceFormatKHR image_format;
  VkPresentModeKHR present_mode;
  VkExtent2D extent;
  VulkanImage depth_attachment;
  /* TODO: is it wise to store framebuffers in here? Maybe it will be better to
   * store it in the renderpass */
  std::vector<GPURenderTarget *> framebuffers;
};