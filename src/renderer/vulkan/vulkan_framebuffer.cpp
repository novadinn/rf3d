#include "vulkan_framebuffer.h"

#include "logger.h"
#include "vulkan_attachment.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_device.h"
#include "vulkan_render_pass.h"

bool VulkanFramebuffer::Create(GPURenderPass *target_render_pass,
                               std::vector<GPUAttachment *> target_attachments,
                               uint32_t target_width, uint32_t target_height) {
  VulkanContext *context = VulkanBackend::GetContext();

  attachments = target_attachments;
  width = target_width;
  height = target_height;

  std::vector<VkImageView> attachment_views;
  for (int i = 0; i < target_attachments.size(); ++i) {
    VulkanAttachment *native_attachment =
        ((VulkanAttachment *)target_attachments[i]);
    attachment_views.push_back(native_attachment->GetImageView());
  }

  VkFramebufferCreateInfo framebuffer_create_info = {};
  framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_create_info.pNext = 0;
  framebuffer_create_info.flags = 0;
  framebuffer_create_info.renderPass =
      ((VulkanRenderPass *)target_render_pass)->GetHandle();
  framebuffer_create_info.attachmentCount = attachment_views.size();
  framebuffer_create_info.pAttachments = attachment_views.data();
  framebuffer_create_info.width = width;
  framebuffer_create_info.height = height;
  framebuffer_create_info.layers = 1;

  VK_CHECK(vkCreateFramebuffer(context->device->GetLogicalDevice(),
                               &framebuffer_create_info, context->allocator,
                               &handle));

  return true;
}

void VulkanFramebuffer::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  vkDestroyFramebuffer(context->device->GetLogicalDevice(), handle,
                       context->allocator);

  handle = 0;
  attachments.clear();
  width = 0;
  height = 0;
}

bool VulkanFramebuffer::Resize(uint32_t new_width, uint32_t new_height) {
  /* TODO: */
  return true;
}