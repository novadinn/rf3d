#include "vulkan_buffer.h"

#include "../../logger.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

#include <string.h>

bool VulkanBuffer::Create(uint64_t buffer_size, VkBufferUsageFlags usage_flags,
                          VkMemoryPropertyFlags memory_flags) {
  VulkanContext *context = VulkanBackend::GetContext();

  total_size = buffer_size;

  VkBufferCreateInfo buffer_create_info = {};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.pNext = 0;
  buffer_create_info.flags = 0;
  buffer_create_info.size = total_size;
  buffer_create_info.usage = usage_flags;
  /* TODO: only used in one queue. */
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_create_info.queueFamilyIndexCount = 0;
  buffer_create_info.pQueueFamilyIndices = 0;

  VK_CHECK(vkCreateBuffer(context->device->GetLogicalDevice(),
                          &buffer_create_info, context->allocator, &handle));

  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(context->device->GetLogicalDevice(), handle,
                                &requirements);
  int memory_index =
      VulkanUtils::FindMemoryIndex(context->device->GetPhysicalDevice(),
                                   requirements.memoryTypeBits, memory_flags);
  if (memory_index == -1) {
    ERROR("Unable to create vulkan buffer because the required memory type "
          "index was not found.");
    return false;
  }

  VkMemoryAllocateInfo allocate_info = {};
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = 0;
  allocate_info.allocationSize = requirements.size;
  allocate_info.memoryTypeIndex = (uint32_t)memory_index;

  VK_CHECK(vkAllocateMemory(context->device->GetLogicalDevice(), &allocate_info,
                            context->allocator, &memory));

  VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(), handle,
                              memory, 0));

  return true;
}

void VulkanBuffer::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  vkFreeMemory(context->device->GetLogicalDevice(), memory, context->allocator);
  vkDestroyBuffer(context->device->GetLogicalDevice(), handle,
                  context->allocator);

  handle = 0;
  memory = 0;
  total_size = 0;
}

void *VulkanBuffer::Lock(uint64_t offset, uint64_t size) {
  VulkanContext *context = VulkanBackend::GetContext();

  void *data;
  VK_CHECK(vkMapMemory(context->device->GetLogicalDevice(), memory, offset,
                       total_size, 0, &data));
  return data;
}

void VulkanBuffer::Unlock() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkUnmapMemory(context->device->GetLogicalDevice(), memory);
}

bool VulkanBuffer::LoadData(uint64_t offset, uint64_t size, void *data) {
  VulkanContext *context = VulkanBackend::GetContext();

  void *data_ptr;
  VK_CHECK(vkMapMemory(context->device->GetLogicalDevice(), memory, offset,
                       size, 0, &data_ptr));
  memcpy(data_ptr, data, size);
  vkUnmapMemory(context->device->GetLogicalDevice(), memory);

  return true;
}

bool VulkanBuffer::LoadDataStaging(uint64_t offset, uint64_t size, void *data) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanBuffer staging_buffer;
  if (!staging_buffer.Create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    ERROR("Failed to create a staging buffer.");
    return false;
  }

  staging_buffer.LoadData(0, staging_buffer.GetSize(), data);
  // VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(),
  //                             staging_buffer.GetHandle(),
  //                             staging_buffer.GetMemory(), 0));
  // VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(), handle,
  //                             memory, 0));

  staging_buffer.CopyTo(this, 0, 0, staging_buffer.GetSize());

  staging_buffer.Destroy();

  return true;
}

bool VulkanBuffer::CopyTo(VulkanBuffer *dest, uint64_t source_offset,
                          uint64_t dest_offset, uint64_t size) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo graphics_queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VkQueue queue = graphics_queue_info.queue;
  VkCommandPool command_pool = graphics_queue_info.command_pool;

  vkQueueWaitIdle(queue);
  VulkanCommandBuffer temp_command_buffer;
  temp_command_buffer.AllocateAndBeginSingleUse(command_pool);

  VkBufferCopy copy_region;
  copy_region.srcOffset = source_offset;
  copy_region.dstOffset = dest_offset;
  copy_region.size = size;

  vkCmdCopyBuffer(temp_command_buffer.GetHandle(), handle, dest->GetHandle(), 1,
                  &copy_region);

  temp_command_buffer.FreeAndEndSingleUse(command_pool, queue);

  return true;
}