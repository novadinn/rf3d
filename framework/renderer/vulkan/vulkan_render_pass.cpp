#include "vulkan_render_pass.h"

#include "../../logger.h"
#include "../gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_command_buffer.h"
#include "vulkan_context.h"
#include "vulkan_debug_marker.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_utils.h"

#include <array>

bool VulkanRenderPass::Create(
    std::vector<GPURenderPassAttachmentConfig> pass_render_attachments,
    glm::vec4 pass_render_area, glm::vec4 pass_clear_color, float pass_depth,
    float pass_stencil, uint8_t pass_clear_flags) {
  VulkanContext *context = VulkanBackend::GetContext();

  attachments = pass_render_attachments;
  render_area = pass_render_area;
  clear_color = pass_clear_color;
  depth = pass_depth;
  stencil = pass_stencil;
  clear_flags = pass_clear_flags;

  std::vector<VkAttachmentDescription> attachment_descriptions;
  std::vector<VkAttachmentReference> color_attachment_references;
  VkAttachmentReference depth_attachment_reference;

  for (uint32_t i = 0; i < attachments.size(); ++i) {
    GPURenderPassAttachmentConfig *attachment_config = &attachments[i];
    bool is_depth_attachment = GPUUtils::IsDepthFormat(attachments[i].format);

    VkAttachmentDescription attachment = {};
    attachment.flags = 0;
    attachment.format =
        VulkanUtils::GPUFormatToVulkanFormat(attachment_config->format);
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    if (!is_depth_attachment) {
      bool do_clear_color = clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_COLOR;

      switch (attachment_config->load_operation) {
      case GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_DONT_CARE: {
        attachment.loadOp = do_clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                           : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      } break;
      case GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_LOAD: {
        attachment.loadOp = do_clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                           : VK_ATTACHMENT_LOAD_OP_LOAD;
      } break;
      default: {
        ERROR("Unsupported attachment load operation!");
        return false;
      } break;
      }
      switch (attachment_config->store_operation) {
      case GPU_RENDER_PASS_ATTACHMENT_STORE_OPERATION_DONT_CARE: {
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      } break;
      case GPU_RENDER_PASS_ATTACHMENT_STORE_OPERATION_STORE: {
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      } break;
      default: {
        ERROR("Unsupported attachment store operation!");
        return false;
      } break;
      }
      attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.initialLayout =
          attachment_config->load_operation ==
                  GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_LOAD
              ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
              : VK_IMAGE_LAYOUT_UNDEFINED;
      attachment.finalLayout = attachment_config->present_after
                                   ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                   : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } else { /* depth attachment */
      bool do_clear_depth = clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_DEPTH;

      switch (attachment_config->load_operation) {
      case GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_DONT_CARE: {
        attachment.loadOp = do_clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                           : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      } break;
      case GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_LOAD: {
        attachment.loadOp = do_clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                           : VK_ATTACHMENT_LOAD_OP_LOAD;
      } break;
      default: {
        ERROR("Unsupported attachment load operation!");
        return false;
      } break;
      }
      switch (attachment_config->store_operation) {
      case GPU_RENDER_PASS_ATTACHMENT_STORE_OPERATION_DONT_CARE: {
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      } break;
      case GPU_RENDER_PASS_ATTACHMENT_STORE_OPERATION_STORE: {
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      } break;
      default: {
        ERROR("Unsupported attachment store operation!");
        return false;
      } break;
      }

      /* TODO: stencil attachments are not supported rn */
      attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.initialLayout =
          attachment_config->load_operation ==
                  GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_LOAD
              ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
              : VK_IMAGE_LAYOUT_UNDEFINED;
      attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    attachment_descriptions.emplace_back(attachment);

    VkAttachmentReference attachment_reference;
    attachment_reference.attachment = i; /* array index */
    attachment_reference.layout =
        is_depth_attachment ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                            : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if (is_depth_attachment) {
      /* only 1 depth attachment */
      depth_attachment_reference = attachment_reference;
    } else {
      color_attachment_references.emplace_back(attachment_reference);
    }
  }

  /* TODO: other attachment types (input, resolve, preserve) */
  VkSubpassDescription subpass_description = {};
  subpass_description.flags = 0;
  subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_description.inputAttachmentCount = 0;
  subpass_description.pInputAttachments = 0;
  subpass_description.colorAttachmentCount = color_attachment_references.size();
  subpass_description.pColorAttachments = color_attachment_references.data();
  subpass_description.pResolveAttachments = 0;
  subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
  subpass_description.preserveAttachmentCount = 0;
  subpass_description.pPreserveAttachments = 0;

  /* TODO: make this configurable */
  std::array<VkSubpassDependency, 2> dependencies;
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo render_pass_create_info = {};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.pNext = 0;
  render_pass_create_info.flags = 0;
  render_pass_create_info.attachmentCount = attachment_descriptions.size();
  render_pass_create_info.pAttachments = attachment_descriptions.data();
  render_pass_create_info.subpassCount = 1;
  render_pass_create_info.pSubpasses = &subpass_description;
  render_pass_create_info.dependencyCount = dependencies.size();
  render_pass_create_info.pDependencies = dependencies.data();

  VK_CHECK(vkCreateRenderPass(context->device->GetLogicalDevice(),
                              &render_pass_create_info, context->allocator,
                              &handle));

  return true;
}

void VulkanRenderPass::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDestroyRenderPass(context->device->GetLogicalDevice(), handle,
                      context->allocator);

  handle = 0;
  attachments.clear();
  render_area = glm::vec4(0.0f);
  clear_color = glm::vec4(0.0f);
  depth = 0;
  stencil = 0;
  clear_flags = 0;
}

void VulkanRenderPass::Begin(GPURenderTarget *target) {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  std::vector<VkClearValue> clear_values;
  if (clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_COLOR) {
    for (int i = 0; i < attachments.size(); ++i) {
      bool is_depth_attachment = GPUUtils::IsDepthFormat(attachments[i].format);
      if (!is_depth_attachment) {
        VkClearValue value = {};
        value.color.float32[0] = clear_color.r;
        value.color.float32[1] = clear_color.g;
        value.color.float32[2] = clear_color.b;
        value.color.float32[3] = clear_color.a;

        clear_values.emplace_back(value);
      }
    }
  }
  if (clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_DEPTH ||
      clear_flags & GPU_RENDER_PASS_CLEAR_FLAG_STENCIL) {
    VkClearValue value = {};
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

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);
  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  vkCmdBeginRenderPass(command_buffer->GetHandle(), &begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport;
  viewport.x = 0.0f;
  viewport.y = render_area.w;
  viewport.width = render_area.z;
  viewport.height = -render_area.w;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor;
  scissor.offset.x = scissor.offset.y = 0;
  scissor.extent.width = render_area.z;
  scissor.extent.height = render_area.w;

  vkCmdSetViewport(command_buffer->GetHandle(), 0, 1, &viewport);
  vkCmdSetScissor(command_buffer->GetHandle(), 0, 1, &scissor);
}

void VulkanRenderPass::End() {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);
  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  vkCmdEndRenderPass(command_buffer->GetHandle());
}

void VulkanRenderPass::SetDebugName(const char *name) {
  VulkanDebugUtils::SetObjectName(name, (uint64_t)handle,
                                  VK_OBJECT_TYPE_RENDER_PASS);
}

void VulkanRenderPass::SetDebugTag(const void *tag, size_t tag_size) {
  VulkanDebugUtils::SetObjectTag(tag, (uint64_t)handle,
                                 VK_OBJECT_TYPE_RENDER_PASS, 0, tag_size);
}