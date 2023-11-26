#include "vulkan_debug_marker.h"

#include "../../logger.h"
#include "vulkan_backend.h"

PFN_vkSetDebugUtilsObjectNameEXT VulkanDebugUtils::vkDebugUtilsSetObjectName;
PFN_vkSetDebugUtilsObjectTagEXT VulkanDebugUtils::vkDebugUtilsSetObjectTag;
PFN_vkCmdBeginDebugUtilsLabelEXT VulkanDebugUtils::vkDebugUtilsBeginRegion;
PFN_vkCmdInsertDebugUtilsLabelEXT VulkanDebugUtils::vkDebugUtilsInsert;
PFN_vkCmdEndDebugUtilsLabelEXT VulkanDebugUtils::vkDebugUtilsEndRegion;
bool VulkanDebugUtils::active;

void VulkanDebugUtils::Initialize() {
  VulkanContext *context = VulkanBackend::GetContext();

  vkDebugUtilsSetObjectName =
      (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(
          context->instance, "vkSetDebugUtilsObjectNameEXT");
  vkDebugUtilsSetObjectTag =
      (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(
          context->instance, "vkSetDebugUtilsObjectTagEXT");
  vkDebugUtilsBeginRegion =
      (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
          context->instance, "vkCmdBeginDebugUtilsLabelEXT");
  vkDebugUtilsInsert = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(
      context->instance, "vkCmdInsertDebugUtilsLabelEXT");
  vkDebugUtilsEndRegion = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(
      context->instance, "vkCmdEndDebugUtilsLabelEXT");

  active = (vkDebugUtilsSetObjectName != VK_NULL_HANDLE);
}

void VulkanDebugUtils::SetObjectName(const char *name, uint64_t object,
                                     VkObjectType object_type) {
  if (active) {
    VulkanContext *context = VulkanBackend::GetContext();

    VkDebugUtilsObjectNameInfoEXT object_name_info = {};
    object_name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    object_name_info.pNext = 0;
    object_name_info.objectType = object_type;
    object_name_info.objectHandle = object;
    object_name_info.pObjectName = name;
    vkDebugUtilsSetObjectName(context->device->GetLogicalDevice(),
                              &object_name_info);
  }
}

void VulkanDebugUtils::SetObjectTag(const void *tag, uint64_t object,
                                    VkObjectType object_type, uint64_t name,
                                    size_t tag_size) {
  if (active) {
    VulkanContext *context = VulkanBackend::GetContext();

    VkDebugUtilsObjectTagInfoEXT object_tag_info = {};
    object_tag_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
    object_tag_info.pNext = 0;
    object_tag_info.objectType = object_type;
    object_tag_info.objectHandle = object;
    object_tag_info.tagName = name;
    object_tag_info.tagSize = tag_size;
    object_tag_info.pTag = tag;

    vkDebugUtilsSetObjectTag(context->device->GetLogicalDevice(),
                             &object_tag_info);
  }
}

void VulkanDebugUtils::BeginRegion(const char *name,
                                   VulkanCommandBuffer *command_buffer,
                                   glm::vec4 color) {
  if (active) {
    VulkanContext *context = VulkanBackend::GetContext();

    VkDebugUtilsLabelEXT label_info = {};
    label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label_info.pNext = 0;
    label_info.pLabelName = name;
    memcpy(label_info.color, &color[0], sizeof(float) * 4);

    vkDebugUtilsBeginRegion(command_buffer->GetHandle(), &label_info);
  }
}

void VulkanDebugUtils::Insert(const char *name,
                              VulkanCommandBuffer *command_buffer,
                              glm::vec4 color) {
  if (active) {
    VulkanContext *context = VulkanBackend::GetContext();

    VkDebugUtilsLabelEXT label_info = {};
    label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label_info.pNext = 0;
    label_info.pLabelName = name;
    memcpy(label_info.color, &color[0], sizeof(float) * 4);

    vkDebugUtilsInsert(command_buffer->GetHandle(), &label_info);
  }
}

void VulkanDebugUtils::EndRegion(VulkanCommandBuffer *command_buffer) {
  if (active) {
    vkDebugUtilsEndRegion(command_buffer->GetHandle());
  }
}
