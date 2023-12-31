#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VulkanContext;
class VulkanRenderPass;
class VulkanCommandBuffer;

struct VulkanPipelineConfig {
  uint32_t stride;
  std::vector<VkVertexInputAttributeDescription> attributes;
  std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
  std::vector<VkPipelineShaderStageCreateInfo> stages;
  std::vector<VkDynamicState> dynamic_states;
  std::vector<VkPushConstantRange> push_constant_ranges;
  VkPrimitiveTopology topology;
  VkViewport viewport;
  VkRect2D scissor;
  bool depth_test_enable;
  bool depth_write_enable;
  uint32_t fragment_output_count;
  /* used for tessellation. 0 if no tessellation is needed */
  uint32_t control_point_count;
};

class VulkanPipeline {
public:
  bool Create(VulkanPipelineConfig *config, VulkanRenderPass *render_pass);
  void Destroy();

  void Bind(VulkanCommandBuffer *command_buffer,
            VkPipelineBindPoint bind_point);

  inline VkPipeline GetHandle() { return handle; }
  inline VkPipelineLayout GetLayout() { return layout; }

private:
  VkPipeline handle;
  VkPipelineLayout layout;
};