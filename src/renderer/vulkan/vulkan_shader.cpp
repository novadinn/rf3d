#include "vulkan_shader.h"

#include "../../logger.h"
#include "renderer/gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

#include <stdio.h>
#include <stdlib.h>

bool VulkanShader::Create(GPUShaderConfig *config, GPURenderPass *render_pass,
                          float viewport_width, float viewport_height) {
  VulkanContext *context = VulkanBackend::GetContext();
  VulkanRenderPass *native_pass = (VulkanRenderPass *)render_pass;

  std::vector<VkShaderModule> stages;
  stages.resize(config->stage_configs.size());
  std::vector<VkPipelineShaderStageCreateInfo> pipeline_stage_create_infos;
  pipeline_stage_create_infos.resize(config->stage_configs.size());

  for (int i = 0; i < stages.size(); ++i) {
    GPUShaderStageConfig *stage_config = &config->stage_configs[i];
    FILE *file = fopen(stage_config->file_path, "rb");
    if (!file) {
      ERROR("Failed to open file %s", stage_config->file_path);
      return false;
    }

    fseek(file, 0, SEEK_END);
    int64_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::vector<uint32_t> file_data;
    file_data.resize(file_size);
    fread(&file_data[0], file_size, 1, file);
    fclose(file);

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = 0;
    create_info.flags = 0;
    create_info.codeSize = file_size;
    create_info.pCode = &file_data[0];

    file_data.clear();

    VK_CHECK(vkCreateShaderModule(context->device->GetLogicalDevice(),
                                  &create_info, 0, &stages[i]));

    pipeline_stage_create_infos[i].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_stage_create_infos[i].pNext = 0;
    pipeline_stage_create_infos[i].flags = 0;
    pipeline_stage_create_infos[i].stage =
        VulkanUtils::GPUShaderStageTypeToVulkanStage(stage_config->type);
    pipeline_stage_create_infos[i].module = stages[i];
    pipeline_stage_create_infos[i].pName = "main";
    /* pipeline_stage_create_infos[i].pSpecializationInfo; */
  }

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = viewport_height;
  viewport.width = viewport_width;
  viewport.height = -viewport_height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = viewport_width;
  scissor.extent.height = viewport_height;

  std::vector<VkVertexInputAttributeDescription> attributes;
  attributes.resize(config->attribute_configs.size());

  uint64_t offset = 0;
  for (int i = 0; i < config->attribute_configs.size(); ++i) {
    GPUShaderAttributeConfig *attr_config = &config->attribute_configs[i];

    attributes[i].location = i;
    attributes[i].binding = 0;
    attributes[i].format =
        VulkanUtils::GPUFormatToVulkanFormat(attr_config->format);
    attributes[i].offset = offset;

    offset += GPUUtils::GetGPUFormatSize(attr_config->format) *
              GPUUtils::GetGPUFormatCount(attr_config->format);
  }

  for (int i = 0; i < config->descriptor_configs.size(); ++i) {
    uniform_buffers.emplace(config->descriptor_configs[i].name,
                            VulkanShaderDescriptor{});
  }
  std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
  descriptor_set_layouts.resize(config->descriptor_configs.size());

  std::vector<VkDescriptorPoolSize> pool_sizes;
  pool_sizes.resize(config->descriptor_configs.size());
  for (int i = 0; i < config->descriptor_configs.size(); ++i) {
    pool_sizes[i].type =
        VulkanUtils::GPUShaderDescriptorTypeToVulkanDescriptorType(
            config->descriptor_configs[i].type);
    pool_sizes[i].descriptorCount = context->swapchain->GetImagesCount();
  }

  int i = 0;
  for (auto it = uniform_buffers.begin(); it != uniform_buffers.end(); ++it) {
    VkDescriptorPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.pNext = 0;
    pool_create_info.flags = 0;
    pool_create_info.maxSets =
        context->swapchain
            ->GetImagesCount(); /* TODO: we can just store a sigle pool for all
                                   of the descriptors * image_count */
    pool_create_info.poolSizeCount = pool_sizes.size();
    pool_create_info.pPoolSizes = &pool_sizes[0];

    VK_CHECK(vkCreateDescriptorPool(context->device->GetLogicalDevice(),
                                    &pool_create_info, context->allocator,
                                    &it->second.descriptor_pool));

    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = i;
    layout_binding.descriptorType =
        VulkanUtils::GPUShaderDescriptorTypeToVulkanDescriptorType(
            config->descriptor_configs[i].type);
    layout_binding.descriptorCount = 1; /* TODO: for array of uniforms */
    layout_binding.stageFlags =
        VulkanUtils::GPUShaderStageFlagsToVulkanShaderStageFlags(
            config->descriptor_configs[i].stage_flags);
    layout_binding.pImmutableSamplers = 0; /* TODO: texture samples */

    VkDescriptorSetLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.pNext = 0;
    layout_create_info.flags = 0;
    layout_create_info.bindingCount = 1;
    layout_create_info.pBindings = &layout_binding;

    VK_CHECK(vkCreateDescriptorSetLayout(
        context->device->GetLogicalDevice(), &layout_create_info,
        context->allocator, &descriptor_set_layouts[i]))

    std::vector<VkDescriptorSetLayout> layouts;
    layouts.resize(context->swapchain->GetImagesCount());
    for (int j = 0; j < layouts.size(); ++j) {
      layouts[j] = descriptor_set_layouts[i];
    }

    VkDescriptorSetAllocateInfo set_allocate_info = {};
    set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_allocate_info.pNext = 0;
    set_allocate_info.descriptorPool = it->second.descriptor_pool;
    set_allocate_info.descriptorSetCount = layouts.size();
    set_allocate_info.pSetLayouts = layouts.data();

    it->second.descriptor_sets.resize(context->swapchain->GetImagesCount());
    VK_CHECK(vkAllocateDescriptorSets(context->device->GetLogicalDevice(),
                                      &set_allocate_info,
                                      it->second.descriptor_sets.data()));

    it->second.descriptor_set_layout = descriptor_set_layouts[i];
    it->second.buffers.resize(context->swapchain->GetImagesCount());

    std::vector<VkDescriptorBufferInfo> buffer_infos;
    buffer_infos.resize(context->swapchain->GetImagesCount());
    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    write_descriptor_sets.resize(context->swapchain->GetImagesCount());

    for (int j = 0; j < context->swapchain->GetImagesCount(); ++j) {
      if (!it->second.buffers[j].Create(GPU_BUFFER_TYPE_UNIFORM,
                                        config->descriptor_configs[i].size)) {
        ERROR("Failed to create a descriptor!");
        return false;
      }

      buffer_infos[j].buffer = it->second.buffers[j].GetHandle();
      buffer_infos[j].offset = 0;
      buffer_infos[j].range = config->descriptor_configs[i].size;

      write_descriptor_sets[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write_descriptor_sets[j].pNext = 0;
      write_descriptor_sets[j].dstSet = it->second.descriptor_sets[j];
      write_descriptor_sets[j].dstBinding = i;
      write_descriptor_sets[j].dstArrayElement = 0;
      write_descriptor_sets[j].descriptorCount = 1;
      write_descriptor_sets[j].descriptorType =
          VulkanUtils::GPUShaderDescriptorTypeToVulkanDescriptorType(
              config->descriptor_configs[i].type);
      write_descriptor_sets[j].pImageInfo = 0;
      write_descriptor_sets[j].pBufferInfo = &buffer_infos[i];
      write_descriptor_sets[j].pTexelBufferView = 0;

      // uniform_buffers[i].buffers[j].Bind(0);
      VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(),
                                  it->second.buffers[j].GetHandle(),
                                  it->second.buffers[j].GetMemory(), 0));

      vkUpdateDescriptorSets(context->device->GetLogicalDevice(), 1,
                             &write_descriptor_sets[j], 0, 0);
    }
    i++;
  }

  /* TODO: empty for now */
  std::vector<VkDynamicState> dynamic_states;

  VulkanPipelineConfig pipeline_config;
  pipeline_config.attributes = attributes;
  pipeline_config.descriptor_set_layouts = descriptor_set_layouts;
  pipeline_config.dynamic_states = dynamic_states;
  pipeline_config.scissor = scissor;
  pipeline_config.stages = pipeline_stage_create_infos;
  pipeline_config.stride = offset;
  pipeline_config.viewport = viewport;

  if (!pipeline.Create(&pipeline_config, native_pass)) {
    return false;
  }

  for (int i = 0; i < stages.size(); ++i) {
    vkDestroyShaderModule(context->device->GetLogicalDevice(), stages[i],
                          context->allocator);
  }

  return true;
}

void VulkanShader::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();
  for (auto it = uniform_buffers.begin(); it != uniform_buffers.end(); ++it) {
    for (int j = 0; j < context->swapchain->GetImagesCount(); ++j) {
      it->second.buffers[j].Destroy();
    }
    vkDestroyDescriptorSetLayout(context->device->GetLogicalDevice(),
                                 it->second.descriptor_set_layout,
                                 context->allocator);
    vkDestroyDescriptorPool(context->device->GetLogicalDevice(),
                            it->second.descriptor_pool, context->allocator);
  }

  pipeline.Destroy();
  pipeline = {};
  uniform_buffers.clear();
}

void VulkanShader::Bind() {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  pipeline.Bind(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

  for (auto it = uniform_buffers.begin(); it != uniform_buffers.end(); ++it) {
    vkCmdBindDescriptorSets(
        command_buffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.GetLayout(), 0, 1,
        &it->second.descriptor_sets[context->image_index], 0, 0);
  }
}

GPUBuffer *VulkanShader::GetDescriptorBuffer(const char *name) {
  VulkanContext *context = VulkanBackend::GetContext();

  return &uniform_buffers[name].buffers[context->image_index];
}