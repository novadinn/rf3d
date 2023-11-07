#include "vulkan_device.h"

#include "../../logger.h"
#include "../../platform.h"
#include "vulkan_backend.h"
#include "vulkan_context.h"

#include <set>
#include <string.h>

bool VulkanDevice::Create(VulkanPhysicalDeviceRequirements *requirements) {
  VulkanContext *context = VulkanBackend::GetContext();

  if (!SelectPhysicalDevice(requirements)) {
    return false;
  }

  for (auto it = queue_infos.begin(); it != queue_infos.end(); ++it) {
    switch (it->first) {
    case VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS: {
      DEBUG("Graphics family index: %d", it->second.family_index);
    } break;
    case VULKAN_DEVICE_QUEUE_TYPE_PRESENT: {
      DEBUG("Present family index: %d", it->second.family_index);
    } break;
    case VULKAN_DEVICE_QUEUE_TYPE_TRANSFER: {
      DEBUG("Transfer family index: %d", it->second.family_index);
    } break;
    }
  }

  /* retrieve unique queue indices */
  std::vector<int> indices;
  std::set<int> unique_queue_indices;

  for (auto it = queue_infos.begin(); it != queue_infos.end(); ++it) {
    VulkanDeviceQueueType type = it->first;
    int index = queue_infos[it->first].family_index;
    if (!unique_queue_indices.contains(index)) {
      indices.emplace_back(index);
    }

    unique_queue_indices.emplace(index);
  }

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  queue_create_infos.resize(indices.size());
  for (int i = 0; i < queue_create_infos.size(); ++i) {
    /* TODO: possibly configurable? */
    float queue_priority = 1.0f;
    queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[i].pNext = 0;
    queue_create_infos[i].flags = 0;
    queue_create_infos[i].queueFamilyIndex = indices[i];
    queue_create_infos[i].queueCount = 1;
    queue_create_infos[i].pQueuePriorities = &queue_priority;
  }

  std::vector<const char *> required_extension_names;
  required_extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef PLATFORM_APPLE
  required_extension_names.emplace_back("VK_KHR_portability_subset");
#endif

  /* TODO: make this part of requirements, and then check if supported via
   * this->features */
  VkPhysicalDeviceFeatures device_features = {};

  VkDeviceCreateInfo device_create_info = {};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pNext = 0;
  device_create_info.flags = 0;
  device_create_info.queueCreateInfoCount = queue_create_infos.size();
  device_create_info.pQueueCreateInfos = queue_create_infos.data();
  device_create_info.enabledLayerCount = 0;   /* deprecated */
  device_create_info.ppEnabledLayerNames = 0; /* deprecated */
  device_create_info.enabledExtensionCount = required_extension_names.size();
  device_create_info.ppEnabledExtensionNames = required_extension_names.data();
  device_create_info.pEnabledFeatures = &device_features;

  VK_CHECK(vkCreateDevice(physical_device, &device_create_info,
                          context->allocator, &logical_device));

  /* Create a queue and a comman buffer for each of the family type. It is
   * better to create only for unique indices and assign data for the others */
  for (auto it = queue_infos.begin(); it != queue_infos.end(); ++it) {
    vkGetDeviceQueue(logical_device, it->second.family_index, 0,
                     &it->second.queue);

    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = 0;
    command_pool_create_info.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = it->second.family_index;

    VK_CHECK(vkCreateCommandPool(logical_device, &command_pool_create_info,
                                 context->allocator, &it->second.command_pool));
  }

  return true;
}

void VulkanDevice::Destroy() {
  VulkanContext *context = VulkanBackend::GetContext();

  for (auto it = queue_infos.begin(); it != queue_infos.end(); ++it) {
    if (it->second.command_buffers.size()) {
      for (int i = 0; i < it->second.command_buffers.size(); ++i) {
        it->second.command_buffers[i].Free(it->second.command_pool);
      }

      it->second.command_buffers.clear();
    }

    vkDestroyCommandPool(logical_device, it->second.command_pool,
                         context->allocator);
  }

  vkDestroyDevice(logical_device, context->allocator);

  physical_device = 0;
  logical_device = 0;

  properties = {};
  features = {};
  memory = {};

  queue_infos.clear();
  swapchain_support_info = {};
  depth_format = VK_FORMAT_UNDEFINED;
}

void VulkanDevice::UpdateSwapchainSupport() {
  VulkanContext *context = VulkanBackend::GetContext();

  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physical_device, context->surface, &swapchain_support_info.capabilities));

  uint32_t format_count = 0;
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physical_device, context->surface, &format_count, 0));

  if (format_count != 0) {
    swapchain_support_info.formats.resize(format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, context->surface, &format_count,
        swapchain_support_info.formats.data()));
  }

  uint32_t present_mode_count = 0;
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device, context->surface, &present_mode_count, 0));
  if (present_mode_count != 0) {
    swapchain_support_info.present_modes.resize(present_mode_count);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, context->surface, &present_mode_count,
        swapchain_support_info.present_modes.data()));
  }
}

void VulkanDevice::UpdateDepthFormat() {
  const int candidate_count = 3;
  VkFormat candidates[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                            VK_FORMAT_D24_UNORM_S8_UINT};

  uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  for (int i = 0; i < candidate_count; ++i) {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i],
                                        &properties);

    if ((properties.linearTilingFeatures & flags) == flags) {
      depth_format = candidates[i];
      return;
    } else if ((properties.optimalTilingFeatures & flags) == flags) {
      depth_format = candidates[i];
      return;
    }
  }

  depth_format = VK_FORMAT_UNDEFINED;
  FATAL("Failed to find a supported depth format!");
}

void VulkanDevice::UpdateCommandBuffers() {
  VulkanContext *context = VulkanBackend::GetContext();

  for (auto it = queue_infos.begin(); it != queue_infos.end(); ++it) {
    it->second.command_buffers.resize(context->swapchain->GetImageCount());
    for (int i = 0; i < it->second.command_buffers.size(); ++i) {
      it->second.command_buffers[i].Allocate(it->second.command_pool,
                                             VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
  }
}

bool VulkanDevice::SupportsDeviceLocalHostVisible() const {
  for (int i = 0; i < memory.memoryTypeCount; ++i) {
    if (((memory.memoryTypes[i].propertyFlags &
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
        ((memory.memoryTypes[i].propertyFlags &
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)) {
      return true;
    }
  }

  return false;
}

bool VulkanDevice::DeviceIsIntegrated() const {
  return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
}

bool VulkanDevice::SelectPhysicalDevice(
    VulkanPhysicalDeviceRequirements *requirements) {
  VulkanContext *context = VulkanBackend::GetContext();

  std::unordered_map<VulkanDeviceQueueType, VulkanDeviceQueueInfo>
      temp_queue_infos;
  VulkanDeviceQueueInfo temp_queue_info = {};
  temp_queue_info.family_index = -1;
  if (requirements->graphics) {
    temp_queue_infos.emplace(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS,
                             temp_queue_info);
  }
  if (requirements->present) {
    temp_queue_infos.emplace(VULKAN_DEVICE_QUEUE_TYPE_PRESENT, temp_queue_info);
  }
  if (requirements->transfer) {
    temp_queue_infos.emplace(VULKAN_DEVICE_QUEUE_TYPE_TRANSFER,
                             temp_queue_info);
  }

  /* Select physical device */
  std::vector<VkPhysicalDevice> physical_devices;
  uint32_t physical_device_count = 0;
  VK_CHECK(
      vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
  if (physical_device_count == 0) {
    ERROR("Failed to find GPU with Vulkan support!");
    return false;
  }
  physical_devices.resize(physical_device_count);
  VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count,
                                      physical_devices.data()));

  for (int i = 0; i < physical_devices.size(); ++i) {
    VkPhysicalDevice current_physical_device = physical_devices[i];

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(current_physical_device, &device_properties);
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(current_physical_device, &device_features);
    VkPhysicalDeviceMemoryProperties device_memory;
    vkGetPhysicalDeviceMemoryProperties(current_physical_device,
                                        &device_memory);

    /* Select queue indices */
    std::vector<VkQueueFamilyProperties> queue_family_properties;
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(current_physical_device,
                                             &queue_family_count, 0);
    queue_family_properties.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(current_physical_device,
                                             &queue_family_count,
                                             &queue_family_properties[0]);

    for (uint32_t j = 0; j < queue_family_count; ++j) {
      VkQueueFamilyProperties queue_properties = queue_family_properties[j];

      if ((queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
          temp_queue_infos.count(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS)) {
        temp_queue_infos[VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS].family_index = j;

        if (temp_queue_infos.count(VULKAN_DEVICE_QUEUE_TYPE_PRESENT)) {
          VkBool32 supports_present = VK_FALSE;
          VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(
              current_physical_device, j, context->surface, &supports_present));
          if (supports_present) {
            temp_queue_infos[VULKAN_DEVICE_QUEUE_TYPE_PRESENT].family_index = j;
          }
        }
      }

      if ((queue_properties.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
          temp_queue_infos.count(VULKAN_DEVICE_QUEUE_TYPE_TRANSFER)) {
        temp_queue_infos[VULKAN_DEVICE_QUEUE_TYPE_TRANSFER].family_index = j;
      }

      /* TODO: other queue types */
    }

    if ((!requirements->graphics ||
         (requirements->graphics &&
          temp_queue_infos[VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS].family_index !=
              -1)) &&
        (!requirements->present ||
         (requirements->present &&
          temp_queue_infos[VULKAN_DEVICE_QUEUE_TYPE_PRESENT].family_index !=
              -1)) &&
        (!requirements->transfer ||
         (requirements->transfer &&
          temp_queue_infos[VULKAN_DEVICE_QUEUE_TYPE_TRANSFER].family_index !=
              -1))) {
      if (!DeviceExtensionsAvailable(
              current_physical_device,
              requirements->device_extension_names.size(),
              requirements->device_extension_names.data())) {
        return false;
      }

      INFO("Device %s meetes requirements", device_properties.deviceName);
      switch (device_properties.deviceType) {
      default:
      case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        INFO("GPU type is Unknown.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        INFO("GPU type is Integrated.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        INFO("GPU type is Descrete.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        INFO("GPU type is Virtual.");
        break;
      case VK_PHYSICAL_DEVICE_TYPE_CPU:
        INFO("GPU type is CPU.");
        break;
      }

      INFO("GPU Driver version: %d.%d.%d",
           VK_VERSION_MAJOR(device_properties.driverVersion),
           VK_VERSION_MINOR(device_properties.driverVersion),
           VK_VERSION_PATCH(device_properties.driverVersion));

      INFO("Vulkan API version: %d.%d.%d",
           VK_VERSION_MAJOR(device_properties.apiVersion),
           VK_VERSION_MINOR(device_properties.apiVersion),
           VK_VERSION_PATCH(device_properties.apiVersion));

      for (int j = 0; j < device_memory.memoryHeapCount; ++j) {
        float memory_size_gib = (((float)device_memory.memoryHeaps[j].size) /
                                 1024.0f / 1024.0f / 1024.0f);
        if (device_memory.memoryHeaps[j].flags &
            VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
          INFO("Local GPU memory: %.2f GiB", memory_size_gib);
        } else {
          INFO("Shared System memory: %.2f GiB", memory_size_gib);
        }
      }

      physical_device = current_physical_device;
      properties = device_properties;
      features = device_features;
      memory = device_memory;
      queue_infos = temp_queue_infos;

      UpdateSwapchainSupport();
      UpdateDepthFormat();

      return true;
    }
  }

  return false;
}

bool VulkanDevice::DeviceExtensionsAvailable(VkPhysicalDevice physical_device,
                                             int required_extension_count,
                                             const char **required_extensions) {
  uint32_t available_extension_count = 0;
  std::vector<VkExtensionProperties> available_extensions;

  VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, 0,
                                                &available_extension_count, 0));
  if (available_extension_count != 0) {
    available_extensions.resize(available_extension_count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, 0,
                                                  &available_extension_count,
                                                  &available_extensions[0]));

    for (int i = 0; i < required_extension_count; ++i) {
      bool found = false;
      for (int j = 0; j < available_extension_count; ++j) {
        if (strcmp(required_extensions[i],
                   available_extensions[j].extensionName)) {
          DEBUG("Required device extension found: %s", required_extensions[i]);
          found = true;
          break;
        }
      }

      if (!found) {
        DEBUG("Required extension not found: '%s', skipping device.",
              required_extensions[i]);
        return false;
      }
    }
  }

  return true;
}