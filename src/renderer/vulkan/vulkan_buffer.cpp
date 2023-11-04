#include "vulkan_buffer.h"

#include "../../logger.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

#include <string.h>

bool VulkanBuffer::Create(GPUBufferType buffer_type, uint64_t buffer_size) {
  VulkanContext *context = VulkanBackend::GetContext();

  total_size = buffer_size;
  type = buffer_type;

  VkBufferUsageFlags buffer_usage;
  VkMemoryPropertyFlags memory_property_flags;

  switch (buffer_type) {
  case GPU_BUFFER_TYPE_VERTEX: {
    buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  } break;
  case GPU_BUFFER_TYPE_INDEX: {
    buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  } break;
  case GPU_BUFFER_TYPE_UNIFORM: {
    uint32_t device_local_bits =
        context->device->SupportsDeviceLocalHostVisible()
            ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            : 0;
    buffer_usage =
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                            device_local_bits;
  } break;
  case GPU_BUFFER_TYPE_STAGING: {
    buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  } break;
  default: {
    ERROR("Unsupported buffer type!");
    return false;
  }
  }

  usage = buffer_usage;

  VkBufferCreateInfo buffer_create_info = {};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.pNext = 0;
  buffer_create_info.flags = 0;
  buffer_create_info.size = total_size;
  buffer_create_info.usage = usage;
  /* TODO: only used in one queue. */
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_create_info.queueFamilyIndexCount = 0;
  buffer_create_info.pQueueFamilyIndices = 0;

  VK_CHECK(vkCreateBuffer(context->device->GetLogicalDevice(),
                          &buffer_create_info, context->allocator, &handle));

  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(context->device->GetLogicalDevice(), handle,
                                &requirements);
  memory_index = VulkanUtils::FindMemoryIndex(
      context->device->GetPhysicalDevice(), requirements.memoryTypeBits,
      memory_property_flags);
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
  usage = 0;
  memory_index = -1;
  type = GPU_BUFFER_TYPE_NONE;
  total_size = 0;
}

bool VulkanBuffer::Bind(uint64_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  switch (type) {
  case GPU_BUFFER_TYPE_INDEX: {
    vkCmdBindIndexBuffer(command_buffer->GetHandle(), handle, offset,
                         VK_INDEX_TYPE_UINT32);
  } break;
  case GPU_BUFFER_TYPE_VERTEX: {
    VkBuffer vertex_buffers[] = {handle};
    VkDeviceSize offsets[] = {offset};

    vkCmdBindVertexBuffers(command_buffer->GetHandle(), 0, 1, vertex_buffers,
                           offsets);
  } break;
  default: {
    ERROR("Failed to bind buffer!");
    return false;
  } break;
  }

  return true;
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

  switch (type) {
  case GPU_BUFFER_TYPE_VERTEX:
  case GPU_BUFFER_TYPE_INDEX: {
    VulkanBuffer staging_buffer;
    if (!staging_buffer.Create(GPU_BUFFER_TYPE_STAGING, total_size)) {
      ERROR("Failed to create a staging buffer.");
      return false;
    }

    staging_buffer.LoadData(0, staging_buffer.GetSize(), data);
    VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(),
                                staging_buffer.GetHandle(),
                                staging_buffer.GetMemory(), 0));
    VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(), handle,
                                memory, 0));

    staging_buffer.CopyTo(this, 0, 0, staging_buffer.GetSize());

    staging_buffer.Destroy();
  } break;
  case GPU_BUFFER_TYPE_STAGING:
  case GPU_BUFFER_TYPE_UNIFORM: {
    void *data_ptr;
    VK_CHECK(vkMapMemory(context->device->GetLogicalDevice(), memory, offset,
                         total_size, 0, &data_ptr));
    memcpy(data_ptr, data, total_size);
    vkUnmapMemory(context->device->GetLogicalDevice(), memory);
  } break;
  default: {
    ERROR("Cannot load gpu buffer data!");
    return false;
  } break;
  }

  return true;
}

bool VulkanBuffer::CopyTo(GPUBuffer *dest, uint64_t source_offset,
                          uint64_t dest_offset, uint64_t size) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo graphics_queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VkQueue queue = graphics_queue_info.queue;
  VkCommandPool command_pool = graphics_queue_info.command_pool;

  vkQueueWaitIdle(queue);
  VulkanCommandBuffer temp_command_buffer;
  temp_command_buffer.Allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  temp_command_buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VkBufferCopy copy_region;
  copy_region.srcOffset = source_offset;
  copy_region.dstOffset = dest_offset;
  copy_region.size = size;

  vkCmdCopyBuffer(temp_command_buffer.GetHandle(), handle,
                  ((VulkanBuffer *)dest)->GetHandle(), 1, &copy_region);

  temp_command_buffer.End();

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = 0;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitSemaphores = 0;
  submit_info.pWaitDstStageMask = 0;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &temp_command_buffer.GetHandle();
  submit_info.signalSemaphoreCount = 0;
  submit_info.pSignalSemaphores = 0;

  VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, 0));

  /* wait for it to finish */
  VK_CHECK(vkQueueWaitIdle(queue));

  temp_command_buffer.Free(command_pool);

  return true;
}