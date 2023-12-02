#include "vulkan_shader.h"

#include "../../logger.h"
#include "../gpu_utils.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"
#include "vulkan_debug_marker.h"
#include "vulkan_descriptor_builder.h"
#include "vulkan_descriptor_set.h"
#include "vulkan_texture.h"
#include "vulkan_utils.h"

#include <map>
#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_beta.h>

bool VulkanShader::Create(std::vector<GPUShaderStageConfig> stage_configs,
                          GPUShaderTopologyType topology_type,
                          uint8_t depth_flags, GPURenderPass *render_pass,
                          float viewport_width, float viewport_height) {
  VulkanContext *context = VulkanBackend::GetContext();
  VulkanRenderPass *native_pass = (VulkanRenderPass *)render_pass;

  std::vector<VkShaderModule> stages;
  stages.resize(stage_configs.size());
  std::vector<VkPipelineShaderStageCreateInfo> pipeline_stage_create_infos;
  pipeline_stage_create_infos.resize(stage_configs.size());

  std::vector<VkPushConstantRange> push_constant_ranges;
  std::vector<VulkanShaderSet> sets;
  std::vector<VkVertexInputAttributeDescription> attributes;
  uint64_t attributes_stride = 0;
  uint32_t fragment_output_count = 0;
  uint32_t tesselation_control_points = 0;

  for (uint32_t i = 0; i < stages.size(); ++i) {
    GPUShaderStageConfig *stage_config = &stage_configs[i];
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
      if (!ReflectVertexAttributes(compiler, resources, attributes,
                                   &attributes_stride)) {
        return false;
      }
    } else if (stage_config->type == GPU_SHADER_STAGE_TYPE_FRAGMENT) {
      fragment_output_count = ReflectFragmentOutputs(compiler, resources);
    } else if (stage_config->type ==
               GPU_SHADER_STAGE_TYPE_TESSELLATION_CONTROL) {
      tesselation_control_points =
          ReflectTesselationControlPoints(compiler, resources);
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
    /* pipeline_stage_create_infos[i].pSpecializationInfo; */ /* TODO:
                                                                 specialization
                                                                 constants */
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

  std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
  for (uint32_t i = 0; i < sets.size(); ++i) {
    VkDescriptorSetLayout set_layout;

    VkDescriptorSetLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.pNext = 0;
    layout_create_info.flags = 0;
    layout_create_info.bindingCount = sets[i].bindings.size();
    layout_create_info.pBindings = sets[i].bindings.data();

    set_layout =
        context->layout_cache->CreateDescriptorLayout(&layout_create_info);

    descriptor_set_layouts.emplace_back(set_layout);
  }

  std::vector<VkDynamicState> dynamic_states;

  VulkanPipelineConfig pipeline_config;
  pipeline_config.attributes = attributes;
  pipeline_config.descriptor_set_layouts = descriptor_set_layouts;
  pipeline_config.dynamic_states = dynamic_states;
  pipeline_config.push_constant_ranges = push_constant_ranges;
  pipeline_config.scissor = scissor;
  pipeline_config.stages = pipeline_stage_create_infos;
  pipeline_config.topology =
      VulkanUtils::GPUShaderTopologyTypeToVulkanTopology(topology_type);
  pipeline_config.stride = attributes_stride;
  pipeline_config.viewport = viewport;
  pipeline_config.depth_test_enable =
      depth_flags & GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE;
  pipeline_config.depth_write_enable =
      depth_flags & GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE;
  pipeline_config.fragment_output_count = fragment_output_count;
  pipeline_config.control_point_count = tesselation_control_points;

  if (!pipeline.Create(&pipeline_config, native_pass)) {
    return false;
  }

  for (uint32_t i = 0; i < stages.size(); ++i) {
    vkDestroyShaderModule(context->device->GetLogicalDevice(), stages[i],
                          context->allocator);
  }

  return true;
}

void VulkanShader::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDeviceWaitIdle(context->device->GetLogicalDevice());

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

void VulkanShader::BindUniformBuffer(GPUDescriptorSet *set, uint32_t offset,
                                     int32_t set_index) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  VulkanDescriptorSet *native_set = (VulkanDescriptorSet *)set;

  vkCmdBindDescriptorSets(command_buffer->GetHandle(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(),
                          set_index, 1, &native_set->GetSet(), 1, &offset);
}

void VulkanShader::BindSampler(GPUDescriptorSet *set, int32_t set_index) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  VulkanDescriptorSet *native_set = (VulkanDescriptorSet *)set;

  vkCmdBindDescriptorSets(command_buffer->GetHandle(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(),
                          set_index, 1, &native_set->GetSet(), 0, 0);
}

void VulkanShader::SetDebugName(const char *name) {
  VulkanDebugUtils::SetObjectName(name, (uint64_t)pipeline.GetHandle(),
                                  VK_OBJECT_TYPE_PIPELINE);
}

void VulkanShader::SetDebugTag(const void *tag, size_t tag_size) {
  VulkanDebugUtils::SetObjectTag(tag, (uint64_t)pipeline.GetHandle(),
                                 VK_OBJECT_TYPE_PIPELINE, 0, tag_size);
}

void VulkanShader::PushConstant(void *value, uint64_t size, uint32_t offset,
                                uint8_t stage_flags) {
  VulkanContext *context = VulkanBackend::GetContext();

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  vkCmdPushConstants(
      command_buffer->GetHandle(), pipeline.GetLayout(),
      VulkanUtils::GPUShaderStageFlagsToVulkanShaderStageFlags(stage_flags),
      offset, size, value);
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
      continue; /* set and binding are duplicated */
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

    sets[set_index].bindings.emplace_back(layout_binding);
  }

  for (auto &image : resources.sampled_images) {
    uint32_t set =
        compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
    uint32_t binding =
        compiler.get_decoration(image.id, spv::DecorationBinding);

    int32_t set_index = -1;
    if (!UpdateDescriptorSetsReflection(sets, set, binding, &set_index)) {
      continue; /* set and binding are duplicated */
    }

    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = binding;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_binding.descriptorCount = 1; /* for array of uniforms */
    layout_binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS; /* TODO: we are
                           not checking in which stages this is presented */
    layout_binding.pImmutableSamplers = 0; /* texture samplers */

    sets[set_index].bindings.emplace_back(layout_binding);
  }
}

void VulkanShader::ReflectStagePushConstantRanges(
    spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources,
    std::vector<VkPushConstantRange> &push_constant_ranges) {
  for (auto &push_constant : resources.push_constant_buffers) {
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

bool VulkanShader::ReflectVertexAttributes(
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
      case 4: {
        attribute_format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attribute_size = sizeof(float) * 4;
      } break;
      default: {
        ERROR("Unknown spirv vector type size!");
        return false;
      } break;
      };
    } break;
    default: {
      ERROR("Unknown spirv type!");
      return false;
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

  return true;
}

uint32_t
VulkanShader::ReflectFragmentOutputs(spirv_cross::Compiler &compiler,
                                     spirv_cross::ShaderResources &resources) {
  uint32_t result = 0;
  for (auto &output : resources.stage_outputs) {
    ++result;
  }

  return result;
}

uint32_t VulkanShader::ReflectTesselationControlPoints(
    spirv_cross::Compiler &compiler, spirv_cross::ShaderResources &resources) {
  auto &entry_point =
      compiler.get_entry_point("main", spv::ExecutionModelTessellationControl);
  return entry_point.output_vertices;
}

bool VulkanShader::UpdateDescriptorSetsReflection(
    std::vector<VulkanShaderSet> &sets, uint32_t set, uint32_t binding,
    int32_t *out_set_index) {
  /* check if binding is duplicated in shader stages */
  for (uint32_t i = 0; i < sets.size(); ++i) {
    if (sets[i].index == set) {
      for (uint32_t j = 0; j < sets[i].bindings.size(); ++j) {
        if (sets[i].bindings[j].binding == binding) {
          return false;
        }
      }
    }
  }

  /* check if set is already presented */
  int32_t set_index = -1;
  for (uint32_t i = 0; i < sets.size(); ++i) {
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