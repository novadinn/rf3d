#include "vulkan_shader_buffer.h"

#include "logger.h"
#include "vulkan_backend.h"
#include "vulkan_utils.h"

void VulkanShaderBuffer::Create(const char *descriptor_name,
                                GPUShaderBufferType descriptor_type,
                                uint8_t descriptor_stage_flags,
                                uint64_t descriptor_size,
                                uint32_t descriptor_index) {
  VulkanContext *context = VulkanBackend::GetContext();

  name = descriptor_name;
  type = descriptor_type;

  /* TODO: create an abstract gpudescriptorpool OR manage them ourselves more
   * properly */
  std::vector<VkDescriptorPoolSize> pool_sizes;
  VkDescriptorPoolSize pool_size;
  pool_size.type = VulkanUtils::GPUShaderBufferTypeToVulkanDescriptorType(type);
  pool_size.descriptorCount = context->swapchain->GetImagesCount();
  pool_sizes.emplace_back(pool_size);

  VkDescriptorPoolCreateInfo pool_create_info = {};
  pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_create_info.pNext = 0;
  pool_create_info.flags = 0;
  pool_create_info.maxSets = context->swapchain->GetImagesCount();
  pool_create_info.poolSizeCount = pool_sizes.size();
  pool_create_info.pPoolSizes = &pool_sizes[0];

  VK_CHECK(vkCreateDescriptorPool(context->device->GetLogicalDevice(),
                                  &pool_create_info, context->allocator,
                                  &pool));

  /* TODO: purely wrong, we need another index (binding index), since there is 2
   * of them: set and binding. Moreother that that, there can be more that 1
   * binding for each descriprotsetlayout */
  VkDescriptorSetLayoutBinding layout_binding = {};
  layout_binding.binding = descriptor_index;
  layout_binding.descriptorType =
      VulkanUtils::GPUShaderBufferTypeToVulkanDescriptorType(type);
  layout_binding.descriptorCount = 1; /* TODO: for array of uniforms */
  layout_binding.stageFlags =
      VulkanUtils::GPUShaderStageFlagsToVulkanShaderStageFlags(
          descriptor_stage_flags);
  layout_binding.pImmutableSamplers = 0; /* texture samples */

  VkDescriptorSetLayoutCreateInfo layout_create_info = {};
  layout_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_create_info.pNext = 0;
  layout_create_info.flags = 0;
  layout_create_info.bindingCount = 1;
  layout_create_info.pBindings = &layout_binding;

  VK_CHECK(vkCreateDescriptorSetLayout(context->device->GetLogicalDevice(),
                                       &layout_create_info, context->allocator,
                                       &set_layout));

  sets.resize(context->swapchain->GetImagesCount());
  buffers.resize(context->swapchain->GetImagesCount());

  for (int i = 0; i < context->swapchain->GetImagesCount(); ++i) {
    VkDescriptorSetAllocateInfo set_allocate_info = {};
    set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_allocate_info.pNext = 0;
    set_allocate_info.descriptorPool = pool;
    set_allocate_info.descriptorSetCount = 1;
    set_allocate_info.pSetLayouts = &set_layout;

    VK_CHECK(vkAllocateDescriptorSets(context->device->GetLogicalDevice(),
                                      &set_allocate_info, &sets[i]));

    buffers[i] = new VulkanBuffer();
    buffers[i]->Create(GPU_BUFFER_TYPE_UNIFORM, descriptor_size);

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = ((VulkanBuffer *)buffers[i])->GetHandle();
    buffer_info.offset = 0;
    buffer_info.range = descriptor_size;

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = 0;
    write_descriptor_set.dstSet = sets[i];
    write_descriptor_set.dstBinding = descriptor_index;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType =
        VulkanUtils::GPUShaderBufferTypeToVulkanDescriptorType(type);
    write_descriptor_set.pImageInfo = 0;
    write_descriptor_set.pBufferInfo = &buffer_info;
    write_descriptor_set.pTexelBufferView = 0;

    VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(),
                                ((VulkanBuffer *)buffers[i])->GetHandle(),
                                ((VulkanBuffer *)buffers[i])->GetMemory(), 0));

    vkUpdateDescriptorSets(context->device->GetLogicalDevice(), 1,
                           &write_descriptor_set, 0, 0);
  }
}

void VulkanShaderBuffer::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  for (int j = 0; j < context->swapchain->GetImagesCount(); ++j) {
    buffers[j]->Destroy();
    delete buffers[j];
  }
  vkDestroyDescriptorSetLayout(context->device->GetLogicalDevice(), set_layout,
                               context->allocator);
  vkDestroyDescriptorPool(context->device->GetLogicalDevice(), pool,
                          context->allocator);
}

void VulkanShaderBuffer::Bind(GPUShader *shader) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  VulkanPipeline &pipeline = ((VulkanShader *)shader)->GetPipeline();

  vkCmdBindDescriptorSets(command_buffer->GetHandle(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(),
                          0, 1, &sets[context->image_index], 0, 0);
}

GPUBuffer *VulkanShaderBuffer::GetBuffer() {
  VulkanContext *context = VulkanBackend::GetContext();

  return buffers[context->image_index];
}