#include "vulkan_shader.h"

#include "../../logger.h"
#include "renderer/gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_descriptor_builder.h"
#include "vulkan_texture.h"
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

  std::vector<VkPushConstantRange> push_constant_ranges;
  std::vector<VulkanShaderSet> sets;
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
    ReflectStagePushConstantRanges(compiler, resources, push_constant_ranges);
    ReflectStageUniforms(compiler, resources, sets);
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

  for (int i = 0; i < sets.size(); ++i) {
    VkDescriptorSetLayout set_layout;

    std::vector<VkDescriptorSetLayoutBinding> native_bindings;
    native_bindings.resize(sets[i].bindings.size());
    for (int j = 0; j < native_bindings.size(); ++j) {
      native_bindings[j] = sets[i].bindings[j].layout_binding;
    }

    VkDescriptorSetLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.pNext = 0;
    layout_create_info.flags = 0;
    layout_create_info.bindingCount = native_bindings.size();
    layout_create_info.pBindings = native_bindings.data();

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

    std::vector<VkDescriptorSet> descriptor_sets;
    descriptor_sets.resize(context->swapchain->GetImageCount());

    for (int i = 0; i < set_layouts.size(); ++i) {
      descriptor_sets[i] = context->descriptor_pools->Allocate(set_layouts[i]);
    }

    sets[i].sets = descriptor_sets;
    sets[i].layout = set_layout;
  }

  shader_sets = sets;

  std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
  for (int i = 0; i < shader_sets.size(); ++i) {
    descriptor_set_layouts.emplace_back(shader_sets[i].layout);
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

  for (int i = 0; i < shader_sets.size(); ++i) {
    /* TODO: those may not be created, assert on that */
    vkDestroyDescriptorSetLayout(context->device->GetLogicalDevice(),
                                 shader_sets[i].layout, context->allocator);
  }

  pipeline.Destroy();
  pipeline = {};
}

void VulkanShader::AttachUniformBuffer(GPUUniformBuffer *uniform_buffer,
                                       uint32_t set, uint32_t binding) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanUniformBuffer *native_uniform_buffer =
      (VulkanUniformBuffer *)uniform_buffer;

  std::vector<VkDescriptorSet> descriptor_sets;
  for (int i = 0; i < shader_sets.size(); ++i) {
    if (shader_sets[i].index == set) {
      descriptor_sets = shader_sets[i].sets;
      break;
    }
  }

  if (descriptor_sets.empty()) {
    ERROR("Failed to attach shader buffer - no such set index exists!");
    return;
  }

  for (int i = 0; i < context->swapchain->GetImageCount(); ++i) {
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = native_uniform_buffer->GetBufferAtFrame(i).GetHandle();
    buffer_info.offset = 0;
    buffer_info.range = native_uniform_buffer->GetDynamicAlignment();

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = 0;
    write_descriptor_set.dstSet = descriptor_sets[i];
    write_descriptor_set.dstBinding = binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write_descriptor_set.pImageInfo = 0;
    write_descriptor_set.pBufferInfo = &buffer_info;
    write_descriptor_set.pTexelBufferView = 0;

    VK_CHECK(vkBindBufferMemory(
        context->device->GetLogicalDevice(),
        native_uniform_buffer->GetBufferAtFrame(i).GetHandle(),
        native_uniform_buffer->GetBufferAtFrame(i).GetMemory(), 0));

    vkUpdateDescriptorSets(context->device->GetLogicalDevice(), 1,
                           &write_descriptor_set, 0, 0);
  }
}

void VulkanShader::AttachTexture(GPUTexture *texture, uint32_t set,
                                 uint32_t binding) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanTexture *native_texture = (VulkanTexture *)texture;

  std::vector<VkDescriptorSet> descriptor_sets;
  for (int i = 0; i < shader_sets.size(); ++i) {
    if (shader_sets[i].index == set) {
      descriptor_sets = shader_sets[i].sets;
      break;
    }
  }

  if (descriptor_sets.empty()) {
    ERROR("Failed to attach shader buffer - no such set index exists!");
    return;
  }

  VkDescriptorImageInfo image_info = {};
  image_info.sampler = native_texture->GetSampler();
  image_info.imageView = native_texture->GetImageView();
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.pNext = 0;
  /* TODO: no need to create a descriptor set for each frame for textures */
  write_descriptor_set.dstSet = descriptor_sets[0];
  write_descriptor_set.dstBinding = binding;
  write_descriptor_set.dstArrayElement = 0;
  write_descriptor_set.descriptorCount = 1;
  write_descriptor_set.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_set.pImageInfo = &image_info;
  write_descriptor_set.pBufferInfo = 0;
  write_descriptor_set.pTexelBufferView = 0;

  vkUpdateDescriptorSets(context->device->GetLogicalDevice(), 1,
                         &write_descriptor_set, 0, 0);
}

void VulkanShader::Bind() {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  pipeline.Bind(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void VulkanShader::BindUniformBuffer(uint32_t set, uint32_t offset) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  std::vector<VkDescriptorSet> descriptor_sets;
  for (int i = 0; i < shader_sets.size(); ++i) {
    if (shader_sets[i].index == set) {
      descriptor_sets = shader_sets[i].sets;
      break;
    }
  }

  if (descriptor_sets.empty()) {
    ERROR("Failed to bind shader buffer - no such set index exists!");
    return;
  }

  vkCmdBindDescriptorSets(command_buffer->GetHandle(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(),
                          set, 1, &descriptor_sets[context->image_index], 1,
                          &offset);
}

void VulkanShader::BindTexture(uint32_t set) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  std::vector<VkDescriptorSet> descriptor_sets;
  for (int i = 0; i < shader_sets.size(); ++i) {
    if (shader_sets[i].index == set) {
      descriptor_sets = shader_sets[i].sets;
      break;
    }
  }

  if (descriptor_sets.empty()) {
    ERROR("Failed to bind shader buffer - no such set index exists!");
    return;
  }

  vkCmdBindDescriptorSets(command_buffer->GetHandle(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(),
                          set, 1, &descriptor_sets[0], 0, 0);
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

void VulkanShader::ReflectStageUniforms(spirv_cross::Compiler &compiler,
                                        spirv_cross::ShaderResources &resources,
                                        std::vector<VulkanShaderSet> &sets) {
  for (auto &buffer : resources.uniform_buffers) {
    uint32_t set =
        compiler.get_decoration(buffer.id, spv::DecorationDescriptorSet);
    uint32_t binding =
        compiler.get_decoration(buffer.id, spv::DecorationBinding);

    int32_t set_index = -1;
    if (!UpdateDescriptorSetsReflection(sets, set, binding, &set_index)) {
      return; /* set and binding are duplicated */
    }

    uint32_t binding_size = 0;
    auto ranges = compiler.get_active_buffer_ranges(buffer.id);
    for (auto &range : ranges) {
      binding_size += range.range;
    }

    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = binding;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layout_binding.descriptorCount = 1; /* for array of uniforms */
    layout_binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS; /* TODO: we are
                           not checking in which stages this is presented */
    layout_binding.pImmutableSamplers = 0; /* texture samplers */

    VulkanShaderBinding shader_binding;
    shader_binding.layout_binding = layout_binding;
    shader_binding.size = binding_size;

    sets[set_index].bindings.emplace_back(shader_binding);
  }

  for (auto &image : resources.sampled_images) {
    uint32_t set =
        compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
    uint32_t binding =
        compiler.get_decoration(image.id, spv::DecorationBinding);

    int32_t set_index = -1;
    if (!UpdateDescriptorSetsReflection(sets, set, binding, &set_index)) {
      return; /* set and binding are duplicated */
    }

    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = binding;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_binding.descriptorCount = 1; /* for array of uniforms */
    layout_binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS; /* TODO: we are
                           not checking in which stages this is presented */
    layout_binding.pImmutableSamplers = 0; /* texture samplers */

    VulkanShaderBinding shader_binding;
    shader_binding.layout_binding = layout_binding;
    shader_binding.size = 0;

    sets[set_index].bindings.emplace_back(shader_binding);
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

bool VulkanShader::UpdateDescriptorSetsReflection(
    std::vector<VulkanShaderSet> &sets, uint32_t set, uint32_t binding,
    int32_t *out_set_index) {
  /* check if binding is duplicated in shader stages */
  for (int i = 0; i < sets.size(); ++i) {
    if (sets[i].index == set) {
      for (int j = 0; j < sets[i].bindings.size(); ++j) {
        if (sets[i].bindings[j].layout_binding.binding == binding) {
          return false;
        }
      }
    }
  }

  /* check if set is already presented */
  int32_t set_index = -1;
  for (int i = 0; i < sets.size(); ++i) {
    if (sets[i].index == set) {
      set_index = i;
      break;
    }
  }
  if (set_index == -1) {
    VulkanShaderSet shader_set;
    shader_set.index = set;
    sets.emplace_back(shader_set);
    set_index = sets.size() - 1;
  }

  *out_set_index = set_index;

  return true;
}