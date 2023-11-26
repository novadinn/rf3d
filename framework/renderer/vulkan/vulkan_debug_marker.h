#pragma once

#include "vulkan_command_buffer.h"

#include <glm/glm.hpp>
#include <stdint.h>
#include <vulkan/vulkan.h>

class VulkanDebugUtils {
public:
  static void Initialize();

  static void SetObjectName(const char *name, uint64_t object,
                            VkObjectType object_type);
  static void SetObjectTag(const void *tag, uint64_t object,
                           VkObjectType object_type, uint64_t name,
                           size_t tag_size);

  static void BeginRegion(const char *name, VulkanCommandBuffer *command_buffer,
                          glm::vec4 color);
  static void Insert(const char *name, VulkanCommandBuffer *command_buffer,
                     glm::vec4 color);
  static void EndRegion(VulkanCommandBuffer *command_buffer);

private:
  static PFN_vkSetDebugUtilsObjectNameEXT vkDebugUtilsSetObjectName;
  static PFN_vkSetDebugUtilsObjectTagEXT vkDebugUtilsSetObjectTag;
  static PFN_vkCmdBeginDebugUtilsLabelEXT vkDebugUtilsBeginRegion;
  static PFN_vkCmdInsertDebugUtilsLabelEXT vkDebugUtilsInsert;
  static PFN_vkCmdEndDebugUtilsLabelEXT vkDebugUtilsEndRegion;
  static bool active;
};