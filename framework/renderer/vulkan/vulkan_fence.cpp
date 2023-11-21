#include "vulkan_fence.h"

#include "../../logger.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_device.h"

void VulkanFence::Create(bool create_signaled) {
  VulkanContext *context = VulkanBackend::GetContext();

  signaled = create_signaled;

  VkFenceCreateInfo fence_create_info = {};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.pNext = 0;
  fence_create_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

  VK_CHECK(vkCreateFence(context->device->GetLogicalDevice(),
                         &fence_create_info, context->allocator, &handle));
}

void VulkanFence::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDestroyFence(context->device->GetLogicalDevice(), handle,
                 context->allocator);

  handle = 0;
  signaled = false;
}

bool VulkanFence::Wait(uint64_t timeout_ns) {
  if (!signaled) {
    VulkanContext *context = VulkanBackend::GetContext();

    VkResult result = vkWaitForFences(context->device->GetLogicalDevice(), 1,
                                      &handle, true, timeout_ns);
    switch (result) {
    case VK_SUCCESS:
      signaled = true;
      return true;
    case VK_TIMEOUT:
      WARN("vkWaitForFences - Timed out");
      break;
    case VK_ERROR_DEVICE_LOST:
      ERROR("vkWaitForFences - VK_ERROR_DEVICE_LOST.");
      break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      ERROR("vkWaitForFences - VK_ERROR_OUT_OF_HOST_MEMORY.");
      break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      ERROR("vkWaitForFences - VK_ERROR_OUT_OF_DEVICE_MEMORY.");
      break;
    default:
      ERROR("vkWaitForFences - An unknown error has occurred.");
      break;
    }
  } else {
    return true;
  }

  return false;
}

void VulkanFence::Reset() {
  if (signaled) {
    VulkanContext *context = VulkanBackend::GetContext();

    VK_CHECK(vkResetFences(context->device->GetLogicalDevice(), 1, &handle));
    signaled = false;
  }
}