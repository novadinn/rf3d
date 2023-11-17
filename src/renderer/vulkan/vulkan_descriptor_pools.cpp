#include "vulkan_descriptor_pools.h"

#include "logger.h"
#include "vulkan_backend.h"

void VulkanDescriptorPools::Initialize() { current_pool = VK_NULL_HANDLE; }

void VulkanDescriptorPools::Shutdown() {
  VulkanContext *context = VulkanBackend::GetContext();

  for (uint32_t i = 0; i < free_pools.size(); ++i) {
    vkDestroyDescriptorPool(context->device->GetLogicalDevice(), free_pools[i],
                            context->allocator);
  }
  for (uint32_t i = 0; i < used_pools.size(); ++i) {
    vkDestroyDescriptorPool(context->device->GetLogicalDevice(), used_pools[i],
                            context->allocator);
  }
}

void VulkanDescriptorPools::Reset() {
  VulkanContext *context = VulkanBackend::GetContext();

  for (uint32_t i = 0; i < used_pools.size(); ++i) {
    vkResetDescriptorPool(context->device->GetLogicalDevice(), used_pools[i],
                          0);
    free_pools.emplace_back(used_pools[i]);
  }

  used_pools.clear();
  current_pool = VK_NULL_HANDLE;
}

VkDescriptorSet VulkanDescriptorPools::Allocate(VkDescriptorSetLayout layout) {
  VulkanContext *context = VulkanBackend::GetContext();

  if (current_pool == VK_NULL_HANDLE) {
    current_pool = GrabPool();
    used_pools.emplace_back(current_pool);
  }

  VkDescriptorSetAllocateInfo set_allocate_info = {};
  set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  set_allocate_info.pNext = 0;
  set_allocate_info.descriptorPool = current_pool;
  set_allocate_info.descriptorSetCount = 1;
  set_allocate_info.pSetLayouts = &layout;

  VkDescriptorSet set;

  VkResult result = vkAllocateDescriptorSets(
      context->device->GetLogicalDevice(), &set_allocate_info, &set);
  switch (result) {
  case VK_SUCCESS: {
    return set;
  } break;
  case VK_ERROR_FRAGMENTED_POOL:
  case VK_ERROR_OUT_OF_POOL_MEMORY: {
    /* reallocate */
    current_pool = GrabPool();
    used_pools.emplace_back(current_pool);
    VK_CHECK(vkAllocateDescriptorSets(context->device->GetLogicalDevice(),
                                      &set_allocate_info, &set));
    return set;
  } break;
  default: {
    FATAL("Unrecoverable error encountered while allocating descriptor set!");
    return 0;
  } break;
  }

  return 0;
}

void VulkanDescriptorPools::Free(VkDescriptorSet descriptor_set) {}

VkDescriptorPool VulkanDescriptorPools::GrabPool() {
  VulkanContext *context = VulkanBackend::GetContext();

  if (free_pools.size() > 0) {
    VkDescriptorPool pool = free_pools.back();
    free_pools.pop_back();
    return pool;
  }

  const uint32_t size_count = 1000;
  const std::vector<std::pair<VkDescriptorType, float>> pool_sizes = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f}};

  std::vector<VkDescriptorPoolSize> sizes;
  sizes.reserve(pool_sizes.size());
  for (auto size : pool_sizes) {
    sizes.push_back({size.first, uint32_t(size.second * size_count)});
  }
  VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
  descriptor_pool_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_create_info.flags = 0;
  descriptor_pool_create_info.maxSets = size_count;
  descriptor_pool_create_info.poolSizeCount = sizes.size();
  descriptor_pool_create_info.pPoolSizes = sizes.data();

  VkDescriptorPool pool;
  VK_CHECK(vkCreateDescriptorPool(context->device->GetLogicalDevice(),
                                  &descriptor_pool_create_info,
                                  context->allocator, &pool));

  return pool;
}