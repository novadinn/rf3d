#include "vulkan_shader.h"

#include "../../logger.h"
#include "renderer/gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

#include <map>
#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_glsl.hpp>
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

  std::vector<VkDescriptorPoolSize> pool_sizes;
  std::vector<VkPushConstantRange> push_constant_ranges;
  std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>
      sets_and_bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;
  uint64_t attributes_stride = 0;

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

    /* reflect the spirv binary */
    spirv_cross::Compiler compiler(file_data.data(),
                                   file_data.size() / sizeof(uint32_t));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    ReflectStagePoolSizes(compiler, resources, pool_sizes);
    ReflectStagePushConstantRanges(compiler, resources, push_constant_ranges);
    ReflectStageBuffers(compiler, resources, sets_and_bindings);
    if (stage_config->type == GPU_SHADER_STAGE_TYPE_VERTEX) {
      ReflectVertexAttributes(compiler, resources, attributes,
                              &attributes_stride);
    }

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

  VkDescriptorPoolCreateInfo pool_create_info = {};
  pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_create_info.pNext = 0;
  pool_create_info.flags = 0;
  pool_create_info.maxSets = 1024; /* TODO: HACK! */
  pool_create_info.poolSizeCount = pool_sizes.size();
  pool_create_info.pPoolSizes = pool_sizes.data();

  VK_CHECK(vkCreateDescriptorPool(context->device->GetLogicalDevice(),
                                  &pool_create_info, context->allocator,
                                  &descriptor_pool));

  for (auto it = sets_and_bindings.begin(); it != sets_and_bindings.end();
       ++it) {
    VkDescriptorSetLayout set_layout;

    VkDescriptorSetLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.pNext = 0;
    layout_create_info.flags = 0;
    layout_create_info.bindingCount = it->second.size();
    layout_create_info.pBindings = it->second.data();

    VK_CHECK(vkCreateDescriptorSetLayout(context->device->GetLogicalDevice(),
                                         &layout_create_info,
                                         context->allocator, &set_layout));

    /* one per frame */
    std::vector<VkDescriptorSetLayout> set_layouts;
    set_layouts.resize(context->swapchain->GetImageCount());

    for (int i = 0; i < set_layouts.size(); ++i) {
      set_layouts[i] =
          set_layout; /* no need to create it multiple times, just copy them */
    }

    std::vector<VkDescriptorSet> sets;
    sets.resize(context->swapchain->GetImageCount());
    std::vector<VulkanBuffer> buffers;
    buffers.resize(context->swapchain->GetImageCount());

    VkDescriptorSetAllocateInfo set_allocate_info = {};
    set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_allocate_info.pNext = 0;
    set_allocate_info.descriptorPool = descriptor_pool;
    set_allocate_info.descriptorSetCount = sets.size();
    set_allocate_info.pSetLayouts = set_layouts.data();

    VK_CHECK(vkAllocateDescriptorSets(context->device->GetLogicalDevice(),
                                      &set_allocate_info, sets.data()));

    GPUShaderBufferIndex index;
    index.set = it->first;
    /* TODO: */
    index.binding = 0;

    VulkanShaderBuffer shader_buffer;
    shader_buffer.layout = set_layout;
    shader_buffer.sets = sets;
    shader_buffer.buffers = buffers;

    uniform_buffers.emplace(index, shader_buffer);
  }

  std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
  for (auto it = uniform_buffers.begin(); it != uniform_buffers.end(); ++it) {
    descriptor_set_layouts.emplace_back(it->second.layout);
  }

  /* TODO: empty for now */
  std::vector<VkDynamicState> dynamic_states;

  VulkanPipelineConfig pipeline_config;
  pipeline_config.attributes = attributes;
  pipeline_config.descriptor_set_layouts = descriptor_set_layouts;
  pipeline_config.dynamic_states = dynamic_states;
  pipeline_config.push_constant_ranges = push_constant_ranges;
  pipeline_config.scissor = scissor;
  pipeline_config.stages = pipeline_stage_create_infos;
  pipeline_config.stride = attributes_stride;
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

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  vkDestroyDescriptorPool(context->device->GetLogicalDevice(), descriptor_pool,
                          context->allocator);
  descriptor_pool = 0;
  for (auto it = uniform_buffers.begin(); it != uniform_buffers.end(); ++it) {
    /* TODO: those may not be created, assert on that */
    for (int i = 0; i < it->second.buffers.size(); ++i) {
      it->second.buffers[i].Destroy();
    }
    vkDestroyDescriptorSetLayout(context->device->GetLogicalDevice(),
                                 it->second.layout, context->allocator);
  }

  pipeline.Destroy();
  pipeline = {};
}

void VulkanShader::PrepareShaderBuffer(GPUShaderBufferIndex index,
                                       uint64_t size, uint32_t element_count) {
  VulkanContext *context = VulkanBackend::GetContext();

  size_t dynamic_alignment = VulkanUtils::GetDynamicAlignment(size);
  size_t buffer_size = element_count * dynamic_alignment;

  VulkanShaderBuffer *shader_buffer = &uniform_buffers[index];
  shader_buffer->dynamic_alignment = dynamic_alignment;

  for (int i = 0; i < context->swapchain->GetImageCount(); ++i) {
    shader_buffer->buffers[i].Create(GPU_BUFFER_TYPE_UNIFORM, buffer_size);

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = shader_buffer->buffers[i].GetHandle();
    buffer_info.offset = 0;
    buffer_info.range = dynamic_alignment;

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = 0;
    write_descriptor_set.dstSet = shader_buffer->sets[i];
    write_descriptor_set.dstBinding = index.binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write_descriptor_set.pImageInfo = 0;
    write_descriptor_set.pBufferInfo = &buffer_info;
    write_descriptor_set.pTexelBufferView = 0;

    VK_CHECK(vkBindBufferMemory(context->device->GetLogicalDevice(),
                                shader_buffer->buffers[i].GetHandle(),
                                shader_buffer->buffers[i].GetMemory(), 0));

    vkUpdateDescriptorSets(context->device->GetLogicalDevice(), 1,
                           &write_descriptor_set, 0, 0);
  }
}

GPUBuffer *VulkanShader::GetShaderBuffer(GPUShaderBufferIndex index) {
  VulkanContext *context = VulkanBackend::GetContext();

  return &(uniform_buffers[index].buffers[context->image_index]);
}

uint32_t VulkanShader::GetShaderBufferAlignment(GPUShaderBufferIndex index) {
  return uniform_buffers[index].dynamic_alignment;
}

void VulkanShader::Bind() {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  pipeline.Bind(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void VulkanShader::BindShaderBuffer(GPUShaderBufferIndex index,
                                    uint32_t draw_index) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  uint32_t dynamic_offsets =
      draw_index * uniform_buffers[index].dynamic_alignment;
  vkCmdBindDescriptorSets(
      command_buffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.GetLayout(), index.set, 1,
      &uniform_buffers[index].sets[context->image_index], 1, &dynamic_offsets);
}

void VulkanShader::PushConstant(GPUShaderPushConstant *push_constant) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  vkCmdPushConstants(command_buffer->GetHandle(), pipeline.GetLayout(),
                     VulkanUtils::GPUShaderStageFlagsToVulkanShaderStageFlags(
                         push_constant->stage_flags),
                     push_constant->offset, push_constant->size,
                     push_constant->value);
}

void VulkanShader::SetTexture(uint32_t index, GPUTexture *texture) {}

void VulkanShader::ReflectStagePoolSizes(
    spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
    std::vector<VkDescriptorPoolSize> &pool_sizes) {
  if (!resources.uniform_buffers.empty()) {
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    pool_size.descriptorCount =
        1024; /* TODO: HACK! max number of descriptors in a pool */
    pool_sizes.emplace_back(pool_size);
  }
}

void VulkanShader::ReflectStageBuffers(
    spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
    std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>
        &sets_and_bindings) {
  for (auto &buffer : resources.uniform_buffers) {
    uint32_t set =
        compiler.get_decoration(buffer.id, spv::DecorationDescriptorSet);
    uint32_t binding =
        compiler.get_decoration(buffer.id, spv::DecorationBinding);

    if (sets_and_bindings.count(set) == 0) {
      sets_and_bindings.emplace(set,
                                std::vector<VkDescriptorSetLayoutBinding>{});
    } else {
      for (int i = 0; i < sets_and_bindings[set].size(); ++i) {
        if (sets_and_bindings[set][i].binding == binding) {
          /* buffer is duplicated in shader stages */
          return;
        }
      }
    }

    uint32_t ubo_size = 0;
    auto ranges = compiler.get_active_buffer_ranges(buffer.id);
    for (auto &range : ranges) {
      ubo_size += range.range;
    }

    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = binding;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layout_binding.descriptorCount = 1; /* for array of uniforms */
    layout_binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS; /* TODO: we are
                           not checking in which stages this is presented */
    layout_binding.pImmutableSamplers = 0; /* texture samplers */

    sets_and_bindings[set].emplace_back(layout_binding);
  }
}

void VulkanShader::ReflectStagePushConstantRanges(
    spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
    std::vector<VkPushConstantRange> &push_constant_ranges) {
  for (auto &push_constant : resources.push_constant_buffers) {
    /* TODO: is a range represented as a full push_constant block, or it is a
     * every member of a block? */
    uint32_t min_offset = UINT32_MAX;
    uint32_t total_size = 0;
    auto ranges = compiler.get_active_buffer_ranges(push_constant.id);
    for (auto &range : ranges) {
      if (range.offset < min_offset) {
        min_offset = range.offset;
      }
      total_size += range.range;
    }

    VkPushConstantRange range = {};
    range.stageFlags =
        VK_SHADER_STAGE_ALL_GRAPHICS; /* TODO: we are not checking in which
                                         stages this is presented */
    range.offset = min_offset;
    range.size = total_size;

    push_constant_ranges.emplace_back(range);
  }
}

void VulkanShader::ReflectVertexAttributes(
    spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
    std::vector<VkVertexInputAttributeDescription> &attributes,
    uint64_t *out_stride) {
  uint32_t offset = 0;
  for (auto &attrib : resources.stage_inputs) {
    uint32_t location =
        compiler.get_decoration(attrib.id, spv::DecorationLocation);

    spirv_cross::SPIRType type = compiler.get_type(attrib.base_type_id);
    VkFormat attribute_format;
    uint32_t attribute_size = 0;

    switch (type.basetype) {
    case spirv_cross::SPIRType::Float: {
      switch (type.vecsize) {
      case 2: {
        attribute_format = VK_FORMAT_R32G32_SFLOAT;
        attribute_size = sizeof(float) * 2;
      } break;
      case 3: {
        attribute_format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_size = sizeof(float) * 3;
      } break;
      default: {
        ERROR("Unknown spirv vector type size!");
      } break;
      };
    } break;
    default: {
      ERROR("Unknown spirv type!");
    } break;
    }

    VkVertexInputAttributeDescription attribute_description = {};
    attribute_description.location = location;
    attribute_description.binding = 0;
    attribute_description.format = attribute_format;
    attribute_description.offset = offset;

    offset += attribute_size;

    attributes.emplace_back(attribute_description);
  }

  *out_stride = offset;
}