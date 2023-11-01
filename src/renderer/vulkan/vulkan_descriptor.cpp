#include "vulkan_descriptor.h"

#include "vulkan_backend.h"
#include "vulkan_utils.h"

void VulkanDescriptor::Create(const char *descriptor_name,
                              GPUDescriptorType descriptor_type,
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
  pool_size.type = VulkanUtils::GPUDescriptorTypeToVulkanDescriptorType(type);
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

  VkDescriptorSetLayoutBinding layout_binding = {};
  layout_binding.binding = descriptor_index;
  layout_binding.descriptorType =
      VulkanUtils::GPUDescriptorTypeToVulkanDescriptorType(type);
  layout_binding.descriptorCount = 1; /* TODO: for array of uniforms */
  layout_binding.stageFlags =
      VulkanUtils::GPUShaderStageFlagsToVulkanShaderStageFlags(
          descriptor_stage_flags);
  layout_binding.pImmutableSamplers = 0; /* TODO: texture samples */

  VkDescriptorSetLayoutCreateInfo layout_create_info = {};
  layout_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_create_info.pNext = 0;
  layout_create_info.flags = 0;
  layout_create_info.bindingCount = 1;
  layout_create_info.pBindings = &layout_binding;

  VK_CHECK(vkCreateDescriptorSetLayout(context->device->GetLogicalDevice(),
                                       &layout_create_info, context->allocator,
                                       &set_layout))

  std::vector<VkDescriptorSetLayout> layouts;
  layouts.resize(context->swapchain->GetImagesCount());
  for (int j = 0; j < layouts.size(); ++j) {
    layouts[j] = set_layout;
  }

  VkDescriptorSetAllocateInfo set_allocate_info = {};
  set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  set_allocate_info.pNext = 0;
  set_allocate_info.descriptorPool = pool;
  set_allocate_info.descriptorSetCount = layouts.size();
  set_allocate_info.pSetLayouts = layouts.data();

  sets.resize(context->swapchain->GetImagesCount());
  VK_CHECK(vkAllocateDescriptorSets(context->device->GetLogicalDevice(),
                                    &set_allocate_info, sets.data()));

  buffers.resize(context->swapchain->GetImagesCount());

  std::vector<VkDescriptorBufferInfo> buffer_infos;
  buffer_infos.resize(context->swapchain->GetImagesCount());
  std::vector<VkWriteDescriptorSet> write_descriptor_sets;
  write_descriptor_sets.resize(context->swapchain->GetImagesCount());

  for (int j = 0; j < context->swapchain->GetImagesCount(); ++j) {
    buffers[j] = new VulkanBuffer();
    buffers[j]->Create(GPU_BUFFER_TYPE_UNIFORM, descriptor_size);

    buffer_infos[j].buffer = ((VulkanBuffer *)buffers[j])->GetHandle();
    buffer_infos[j].offset = 0;
    buffer_infos[j].range = descriptor_size;

    write_descriptor_sets[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_sets[j].pNext = 0;
    write_descriptor_sets[j].dstSet = sets[descriptor_index];
    write_descriptor_sets[j].dstBinding = descriptor_index;
    write_descriptor_sets[j].dstArrayElement = 0;
    write_descriptor_sets[j].descriptorCount = 1;
    write_descriptor_sets[j].descriptorType =
        VulkanUtils::GPUDescriptorTypeToVulkanDescriptorType(type);
    write_descriptor_sets[j].pImageInfo = 0;
    write_descriptor_sets[j].pBufferInfo = &buffer_infos[descriptor_index];
    write_descriptor_sets[j].pTexelBufferView = 0;

    VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(),
                                ((VulkanBuffer *)buffers[j])->GetHandle(),
                                ((VulkanBuffer *)buffers[j])->GetMemory(), 0));

    vkUpdateDescriptorSets(context->device->GetLogicalDevice(), 1,
                           &write_descriptor_sets[j], 0, 0);
  }
}

void VulkanDescriptor::Destroy() {
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

void VulkanDescriptor::Bind(GPUShader *shader) {
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

GPUBuffer *VulkanDescriptor::GetBuffer() {
  VulkanContext *context = VulkanBackend::GetContext();

  return buffers[context->image_index];
}