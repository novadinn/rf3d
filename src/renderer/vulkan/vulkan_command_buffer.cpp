#include "vulkan_command_buffer.h"

#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_device.h"

void VulkanCommandBuffer::Allocate(VkCommandPool command_pool,
                                   VkCommandBufferLevel level) {
  VulkanContext *context = VulkanBackend::GetContext();

  VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
  command_buffer_allocate_info.sType =
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_allocate_info.pNext = 0;
  command_buffer_allocate_info.commandPool = command_pool;
  command_buffer_allocate_info.level = level;
  command_buffer_allocate_info.commandBufferCount = 1;

  VK_CHECK(vkAllocateCommandBuffers(context->device->GetLogicalDevice(),
                                    &command_buffer_allocate_info, &handle));
}

void VulkanCommandBuffer::Free(VkCommandPool command_pool) {
  VulkanContext *context = VulkanBackend::GetContext();

  vkFreeCommandBuffers(context->device->GetLogicalDevice(), command_pool, 1,
                       &handle);

  handle = 0;
}

void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags usage) {
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = 0;
  begin_info.flags = usage;
  begin_info.pInheritanceInfo = 0;

  VK_CHECK(vkBeginCommandBuffer(handle, &begin_info));
}

void VulkanCommandBuffer::End() { VK_CHECK(vkEndCommandBuffer(handle)); }

void VulkanCommandBuffer::AllocateAndBeginSingleUse(
    VkCommandPool command_pool) {
  Allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void VulkanCommandBuffer::FreeAndEndSingleUse(VkCommandPool command_pool,
                                              VkQueue queue) {
  End();

  VkSubmitInfo submit_info;
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = 0;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitSemaphores = 0;
  submit_info.pWaitDstStageMask = 0;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &handle;
  submit_info.signalSemaphoreCount = 0;
  submit_info.pSignalSemaphores = 0;

  VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, 0));
  VK_CHECK(vkQueueWaitIdle(queue));

  Free(command_pool);
}

void VulkanCommandBuffer::Reset(VkCommandBufferResetFlags flags) {
  VK_CHECK(vkResetCommandBuffer(handle, flags));
}