#include "vulkan_shader.h"

#include "../../logger.h"
#include "renderer/gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_descriptor.h"
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

  std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
  descriptor_set_layouts.resize(config->descriptors.size());
  for (int i = 0; i < descriptor_set_layouts.size(); ++i) {
    descriptor_set_layouts[i] =
        ((VulkanDescriptor *)config->descriptors[i])->GetSetLayout();
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

  pipeline.Destroy();
  pipeline = {};
}

void VulkanShader::Bind() {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  pipeline.Bind(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
}