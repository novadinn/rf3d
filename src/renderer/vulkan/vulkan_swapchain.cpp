#include "vulkan_swapchain.h"

#include "../../logger.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_device.h"
#include "vulkan_texture.h"

#include <glm/glm.hpp>

bool VulkanSwapchain::Create(uint32_t width, uint32_t height) {
  VulkanContext *context = VulkanBackend::GetContext();

  /* requery swapchain support and depth format, since after swapchain
   * recreation it may not be valid */
  context->device->UpdateSwapchainSupport();
  context->device->UpdateDepthFormat();

  VulkanSwapchainSupportInfo swapchain_support_info =
      context->device->GetSwapchainSupportInfo();

  /* Select optimal format and present mode. TODO: maybe configurable */
  image_format = swapchain_support_info.formats[0];
  for (int i = 0; i < swapchain_support_info.formats.size(); ++i) {
    VkSurfaceFormatKHR format = swapchain_support_info.formats[i];
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      image_format = format;
      break;
    }
  }

  present_mode = swapchain_support_info.present_modes[0];
  for (int i = 0; i < swapchain_support_info.present_modes.size(); ++i) {
    VkPresentModeKHR mode = swapchain_support_info.present_modes[i];
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode = mode;
      break;
    }
  }

  extent = {width, height};
  if (swapchain_support_info.capabilities.currentExtent.width != UINT32_MAX) {
    extent = swapchain_support_info.capabilities.currentExtent;
  }
  extent.width = glm::clamp(
      extent.width, swapchain_support_info.capabilities.minImageExtent.width,
      swapchain_support_info.capabilities.maxImageExtent.width);
  extent.height = glm::clamp(
      extent.height, swapchain_support_info.capabilities.minImageExtent.height,
      swapchain_support_info.capabilities.maxImageExtent.height);

  int image_count = swapchain_support_info.capabilities.minImageCount + 1;
  if (swapchain_support_info.capabilities.maxImageCount > 0 &&
      image_count > swapchain_support_info.capabilities.maxImageCount) {
    image_count = swapchain_support_info.capabilities.maxImageCount;
  }

  max_frames_in_flight = image_count - 1;

  VulkanDeviceQueueInfo graphics_queue_info = {};
  if (!context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS,
                                     &graphics_queue_info)) {
    ERROR("Can not create a swapchain without graphics queue support!");
    return false;
  }
  VulkanDeviceQueueInfo present_queue_info = {};
  if (!context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_PRESENT,
                                     &present_queue_info)) {
    ERROR("Can not create a swapchain without present queue support!");
    return false;
  }

  VkSwapchainCreateInfoKHR swapchain_create_info = {};
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.pNext = 0;
  swapchain_create_info.flags = 0;
  swapchain_create_info.surface = context->surface;
  swapchain_create_info.minImageCount = image_count;
  swapchain_create_info.imageFormat = image_format.format;
  swapchain_create_info.imageColorSpace = image_format.colorSpace;
  swapchain_create_info.imageExtent = extent;
  swapchain_create_info.imageArrayLayers = 1;
  swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  if (graphics_queue_info.family_index != present_queue_info.family_index) {
    uint32_t indices[] = {(uint32_t)graphics_queue_info.family_index,
                          (uint32_t)present_queue_info.family_index};
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 2;
    swapchain_create_info.pQueueFamilyIndices = indices;
  } else {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = 0;
  }
  swapchain_create_info.preTransform =
      swapchain_support_info.capabilities.currentTransform;
  swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_create_info.presentMode = present_mode;
  swapchain_create_info.clipped = VK_TRUE;
  swapchain_create_info.oldSwapchain = 0;

  VK_CHECK(vkCreateSwapchainKHR(context->device->GetLogicalDevice(),
                                &swapchain_create_info, context->allocator,
                                &handle));

  uint32_t swapchain_image_count = 0;
  VK_CHECK(vkGetSwapchainImagesKHR(context->device->GetLogicalDevice(), handle,
                                   &swapchain_image_count, 0));
  images.resize(swapchain_image_count);
  image_views.resize(swapchain_image_count);
  framebuffers.resize(swapchain_image_count);
  VK_CHECK(vkGetSwapchainImagesKHR(context->device->GetLogicalDevice(), handle,
                                   &swapchain_image_count, images.data()));

  for (int i = 0; i < swapchain_image_count; ++i) {
    /* TODO: rendertargetallocate */
    framebuffers[i] = new VulkanFramebuffer();

    VkImage image = images[i];

    VkImageViewCreateInfo view_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = image_format.format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(context->device->GetLogicalDevice(), &view_info,
                               context->allocator, &image_views[i]));
  }

  depth_attachment.Create(
      extent.width, extent.height, context->device->GetDepthFormat(),
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
  depth_attachment.CreateImageView(context->device->GetDepthFormat(),
                                   VK_IMAGE_ASPECT_DEPTH_BIT);

  return true;
}

void VulkanSwapchain::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  depth_attachment.Destroy();

  for (int i = 0; i < image_views.size(); ++i) {
    vkDestroyImageView(context->device->GetLogicalDevice(), image_views[i],
                       context->allocator);
  }

  for (int i = 0; i < framebuffers.size(); ++i) {
    framebuffers[i]->Destroy();
    delete framebuffers[i];
  }

  vkDestroySwapchainKHR(context->device->GetLogicalDevice(), handle,
                        context->allocator);
}

bool VulkanSwapchain::Recreate(uint32_t width, uint32_t height) {
  Destroy();
  return Create(width, height);
}

bool VulkanSwapchain::AcquireNextImage(uint64_t timeout_ns,
                                       VkSemaphore semaphor, VkFence fence,
                                       uint32_t width, uint32_t height,
                                       uint32_t *out_image_index) {
  VulkanContext *context = VulkanBackend::GetContext();

  VkResult result =
      vkAcquireNextImageKHR(context->device->GetLogicalDevice(), handle,
                            timeout_ns, semaphor, fence, out_image_index);

  /* TODO: hardcoded */
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    Recreate(width, height);
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    FATAL("Failed to acquire swapchain image!");
    return false;
  }

  return true;
}

void VulkanSwapchain::RegenerateFramebuffers(VulkanRenderPass *render_pass,
                                             uint32_t width, uint32_t height) {
  for (int i = 0; i < framebuffers.size(); ++i) {
    // std::vector<VkImageView> attachments = {image_views[i],
    //                                         depth_attachment.GetImageView()};
    /* TODO: horrible... */
    VulkanImage color_image;
    color_image.SetImageView(image_views[i]);
    VulkanImage depth_image;
    depth_image.SetImageView(depth_attachment.GetImageView());
    VulkanTexture color_texture;
    color_texture.SetImage(color_image);
    VulkanTexture depth_texture;
    depth_texture.SetImage(depth_image);

    std::vector<GPUTexture *> attachments;
    attachments.emplace_back(&color_texture);
    attachments.emplace_back(&depth_texture);

    framebuffers[i]->Create(render_pass, attachments, width, height);
  }
}