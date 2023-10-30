#include "vulkan_render_pass.h"

#include "vulkan_backend.h"
#include "vulkan_command_buffer.h"
#include "vulkan_context.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"

void VulkanRenderPass::Create(
    std::vector<GPURenderTarget *> pass_render_targets,
    glm::vec4 pass_render_area, glm::vec4 pass_clear_color, float pass_depth,
    float pass_stencil, uint8_t pass_clear_flags) {
  /* TODO: fully hardcoded */
  VulkanContext *context = VulkanBackend::GetContext();

  render_targets = pass_render_targets;
  render_area = pass_render_area;
  clear_color = pass_clear_color;
  depth = pass_depth;
  stencil = pass_stencil;
  clear_flags = pass_clear_flags;

  uint32_t attachment_description_count = 2;
  std::vector<VkAttachmentDescription> attachment_descriptions;
  attachment_descriptions.reserve(attachment_description_count);

  /* TODO: add possibility to specify attachments externally */
  /* TODO: add is_present or smth instead of this bool */
  static bool is_main_render_pass = true;
  VkAttachmentDescription color_attachment = {};
  color_attachment.flags = 0;
  color_attachment.format = context->swapchain->GetImageFormat().format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = is_main_render_pass
                                     ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                     : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  is_main_render_pass = false;

  attachment_descriptions[0] = color_attachment;

  VkAttachmentReference color_attachment_reference;
  color_attachment_reference.attachment = 0; /* array index */
  color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depth_attachment = {};
  depth_attachment.flags = 0;
  depth_attachment.format = context->device->GetDepthFormat();
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  attachment_descriptions[1] = depth_attachment;

  VkAttachmentReference depth_attachment_reference;
  depth_attachment_reference.attachment = 1; /* array index */
  depth_attachment_reference.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  /* TODO: other attachment types (input, resolve, preserve) */

  VkSubpassDescription subpass_description = {};
  subpass_description.flags = 0;
  subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_description.inputAttachmentCount = 0;
  subpass_description.pInputAttachments = 0;
  subpass_description.colorAttachmentCount = 1;
  subpass_description.pColorAttachments = &color_attachment_reference;
  subpass_description.pResolveAttachments = 0;
  subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
  subpass_description.preserveAttachmentCount = 0;
  subpass_description.pPreserveAttachments = 0;

  VkSubpassDependency subpass_dependency = {};
  subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependency.dstSubpass = 0;
  subpass_dependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.srcAccessMask = 0;
  subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpass_dependency.dependencyFlags = 0;

  /* TODO: create a depth attachment subpass dependency, dont care for now */

  VkRenderPassCreateInfo render_pass_create_info = {};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.pNext = 0;
  render_pass_create_info.flags = 0;
  render_pass_create_info.attachmentCount = attachment_description_count;
  render_pass_create_info.pAttachments = &attachment_descriptions[0];
  render_pass_create_info.subpassCount = 1;
  render_pass_create_info.pSubpasses = &subpass_description;
  render_pass_create_info.dependencyCount = 1;
  render_pass_create_info.pDependencies = &subpass_dependency;

  VK_CHECK(vkCreateRenderPass(context->device->GetLogicalDevice(),
                              &render_pass_create_info, context->allocator,
                              &handle));
}

void VulkanRenderPass::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDestroyRenderPass(context->device->GetLogicalDevice(), handle,
                      context->allocator);
}

void VulkanRenderPass::Begin(GPURenderTarget *target) {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  std::vector<VkClearValue> clear_values;
  if (clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_COLOR) {
    VkClearValue value;
    value.color.float32[0] = clear_color.r;
    value.color.float32[1] = clear_color.g;
    value.color.float32[2] = clear_color.b;
    value.color.float32[3] = clear_color.a;

    clear_values.emplace_back(value);
  }
  if (clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_DEPTH ||
      clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_STENCIL) {
    VkClearValue value;
    if (clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_DEPTH) {
      value.depthStencil.depth = depth;
    }
    if (clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_STENCIL) {
      value.depthStencil.stencil = stencil;
    }

    clear_values.emplace_back(value);
  }

  VkRenderPassBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  begin_info.pNext = 0;
  begin_info.renderPass = handle;
  begin_info.framebuffer = ((VulkanFramebuffer *)target)->GetHandle();
  begin_info.renderArea.offset.x = render_area.x;
  begin_info.renderArea.offset.y = render_area.y;
  begin_info.renderArea.extent.width = render_area.z;
  begin_info.renderArea.extent.height = render_area.w;
  begin_info.clearValueCount = clear_values.size();
  begin_info.pClearValues = clear_values.data();

  VulkanDeviceQueueInfo info;
  context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS, &info);
  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  vkCmdBeginRenderPass(command_buffer->GetHandle(), &begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderPass::End() {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info;
  context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS, &info);
  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  vkCmdEndRenderPass(command_buffer->GetHandle());
}