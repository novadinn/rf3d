#include "vulkan_backend.h"

#include "logger.h"
#include "platform.h"
#include "renderer/gpu_shader.h"
#include "vulkan_attribute_array.h"
#include "vulkan_render_pass.h"
#include "vulkan_texture.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

VulkanContext *VulkanBackend::context;

static bool createVulkanInstance(VkApplicationInfo application_info,
                                 SDL_Window *window, VkInstance *out_instance);
static bool createVulkanSurface(SDL_Window *window, VkSurfaceKHR *out_surface);
static bool
createVulkanDebugMessanger(VkDebugUtilsMessengerEXT *out_debug_messenger);
static bool requiredLayersAvailable(std::vector<const char *> required_layers);
static bool
requiredExtensionsAvailable(std::vector<const char *> required_extensions);
VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);

bool VulkanBackend::Initialize(SDL_Window *sdl_window) {
  context = new VulkanContext();
  window = sdl_window;

  context->image_index = 0;
  context->current_frame = 0;

  VkApplicationInfo application_info = {};
  application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  application_info.pNext = nullptr;
  application_info.pApplicationName = "Shade a Sphere";
  application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.pEngineName = "Shade a Sphere";
  application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.apiVersion = VK_API_VERSION_1_3;

  if (!createVulkanInstance(application_info, window, &context->instance)) {
    return false;
  }

#ifndef NDEBUG
  createVulkanDebugMessanger(&context->debug_messenger);
#endif

  if (!createVulkanSurface(window, &context->surface)) {
    return false;
  }

  context->device = new VulkanDevice();
  VulkanPhysicalDeviceRequirements requirements;
  requirements.device_extension_names.emplace_back(
      VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  requirements.graphics = true;
  requirements.present = true;
  requirements.transfer = true;
  if (!context->device->Create(&requirements)) {
    return false;
  }

  /* TODO: temp */
  int width, height;
  SDL_Vulkan_GetDrawableSize(window, &width, &height);

  context->swapchain = new VulkanSwapchain();
  if (!context->swapchain->Create(width, height)) {
    return false;
  }

  main_render_pass = RenderPassAllocate();
  main_render_pass->Create(
      std::vector<GPURenderTarget *>{}, glm::vec4(0, 0, width, height),
      glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), 1.0f, 0.0f,
      GPU_RENDER_PASS_CLEAR_FLAG_COLOR | GPU_RENDER_PASS_CLEAR_FLAG_DEPTH |
          GPU_RENDER_PASS_CLEAR_FLAG_STENCIL);

  context->swapchain->RegenerateFramebuffers(
      (VulkanRenderPass *)main_render_pass, width, height);

  context->device->UpdateCommandBuffers();

  context->image_available_semaphores.resize(
      context->swapchain->GetMaxFramesInFlights());
  context->queue_complete_semaphores.resize(
      context->swapchain->GetMaxFramesInFlights());
  context->in_flight_fences.resize(context->swapchain->GetMaxFramesInFlights());
  for (int i = 0; i < context->swapchain->GetMaxFramesInFlights(); ++i) {
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = 0;
    semaphore_create_info.flags = 0;

    VK_CHECK(vkCreateSemaphore(context->device->GetLogicalDevice(),
                               &semaphore_create_info, context->allocator,
                               &context->image_available_semaphores[i]));
    VK_CHECK(vkCreateSemaphore(context->device->GetLogicalDevice(),
                               &semaphore_create_info, context->allocator,
                               &context->queue_complete_semaphores[i]));

    context->in_flight_fences[i] = new VulkanFence();
    context->in_flight_fences[i]->Create(true);
  }

  context->images_in_flight.resize(context->swapchain->GetImagesCount());
  for (int i = 0; i < context->images_in_flight.size(); ++i) {
    context->images_in_flight[i] = 0;
  }

  std::vector<GPUShaderStageConfig> stage_configs;
  stage_configs.resize(2);
  stage_configs[0].type = GPU_SHADER_STAGE_TYPE_VERTEX;
  stage_configs[0].file_path = "assets/shaders/object_shader.vert.spv";
  stage_configs[1].type = GPU_SHADER_STAGE_TYPE_FRAGMENT;
  stage_configs[1].file_path = "assets/shaders/object_shader.frag.spv";

  return true;
}

void VulkanBackend::Shutdown() {
  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  for (int i = 0; i < context->swapchain->GetMaxFramesInFlights(); ++i) {
    vkDestroySemaphore(context->device->GetLogicalDevice(),
                       context->image_available_semaphores[i],
                       context->allocator);
    vkDestroySemaphore(context->device->GetLogicalDevice(),
                       context->queue_complete_semaphores[i],
                       context->allocator);
    context->in_flight_fences[i]->Destroy();
    delete context->in_flight_fences[i];
  }

  main_render_pass->Destroy();
  delete main_render_pass;
  main_render_pass = 0;

  context->swapchain->Destroy();
  delete context->swapchain;

  context->device->Destroy();
  delete context->device;

  vkDestroySurfaceKHR(context->instance, context->surface, context->allocator);

#ifndef NDEBUG
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          context->instance, "vkDestroyDebugUtilsMessengerEXT");
  func(context->instance, context->debug_messenger, context->allocator);
#endif

  vkDestroyInstance(context->instance, context->allocator);

  delete context;
  context = 0;
  window = 0;
}

void VulkanBackend::Resize(uint32_t width, uint32_t height) {}

bool VulkanBackend::BeginFrame() {
  vkDeviceWaitIdle(context->device->GetLogicalDevice());

  if (!context->in_flight_fences[context->current_frame]->Wait(UINT64_MAX)) {
    return false;
  }

  glm::vec4 render_area = main_render_pass->GetRenderArea();
  if (!context->swapchain->AcquireNextImage(
          UINT64_MAX,
          context->image_available_semaphores[context->current_frame], 0,
          render_area.z, render_area.w, &context->image_index)) {
    return false;
  }

  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];
  command_buffer->Begin(0);

  VkViewport viewport;
  viewport.x = 0.0f;
  viewport.y = render_area.w;
  viewport.width = render_area.z;
  viewport.height = -render_area.w;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor;
  scissor.offset.x = scissor.offset.y = 0;
  scissor.extent.width = render_area.z;
  scissor.extent.height = render_area.w;

  vkCmdSetViewport(command_buffer->GetHandle(), 0, 1, &viewport);
  vkCmdSetScissor(command_buffer->GetHandle(), 0, 1, &scissor);

  return true;
}

bool VulkanBackend::EndFrame() {
  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  command_buffer->End();

  /* make sure the previous frame is not using this image (its fence is
   * being waited on) */
  if (context->images_in_flight[context->image_index] != VK_NULL_HANDLE) {
    context->images_in_flight[context->image_index]->Wait(UINT64_MAX);
  }

  /* mark the image fence as in-use by this frame */
  context->images_in_flight[context->image_index] =
      context->in_flight_fences[context->current_frame];

  context->in_flight_fences[context->current_frame]->Reset();

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = 0;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores =
      &context->image_available_semaphores[context->current_frame];
  submit_info.pWaitDstStageMask = 0;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer->GetHandle();
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores =
      &context->queue_complete_semaphores[context->current_frame];

  /* each semaphore waits on the corresponding pipeline stage to complete. 1:1
   * ratio. VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent
   * colour attachment writes from executing until the semaphore signals (i.e.
   * one frame is presented at a time) */
  VkPipelineStageFlags flags[1] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.pWaitDstStageMask = flags;

  VulkanDeviceQueueInfo graphics_queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);
  VulkanDeviceQueueInfo present_queue_info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_PRESENT);

  VkResult result = vkQueueSubmit(
      graphics_queue_info.queue, 1, &submit_info,
      context->in_flight_fences[context->current_frame]->GetHandle());
  if (result != VK_SUCCESS) {
    ERROR("Vulkan queue submit failed.");
    return false;
  }

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = 0;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores =
      &context->queue_complete_semaphores[context->current_frame];
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &context->swapchain->GetHandle();
  present_info.pImageIndices = &context->image_index;
  present_info.pResults = 0;

  glm::vec4 render_area = main_render_pass->GetRenderArea();
  result = vkQueuePresentKHR(present_queue_info.queue, &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    context->swapchain->Recreate(render_area.z, render_area.w);
  } else if (result != VK_SUCCESS) {
    FATAL("Failed to present swap chain image!");
    return false;
  }

  context->current_frame = (context->current_frame + 1) %
                           context->swapchain->GetMaxFramesInFlights();

  return true;
}

bool VulkanBackend::Draw(uint32_t element_count) {
  VulkanDeviceQueueInfo info =
      context->device->GetQueueInfo(VULKAN_DEVICE_QUEUE_TYPE_GRAPHICS);

  VulkanCommandBuffer *command_buffer =
      &info.command_buffers[context->image_index];

  vkCmdDraw(command_buffer->GetHandle(), element_count, 1, 0, 0);

  return true;
}

GPURenderPass *VulkanBackend::GetWindowRenderPass() { return main_render_pass; }

GPURenderTarget *VulkanBackend::GetCurrentWindowRenderTarget() {
  return context->swapchain->GetFramebuffers()[context->image_index];
}

GPUBuffer *VulkanBackend::BufferAllocate() { return new VulkanBuffer(); }

GPURenderTarget *VulkanBackend::RenderTargetAllocate() {
  return new VulkanFramebuffer();
}

GPURenderPass *VulkanBackend::RenderPassAllocate() {
  return new VulkanRenderPass();
}

GPUShader *VulkanBackend::ShaderAllocate() { return new VulkanShader(); }

GPUAttributeArray *VulkanBackend::AttributeArrayAllocate() {
  return new VulkanAttributeArray();
}

GPUTexture *VulkanBackend::TextureAllocate() { return new VulkanTexture(); }

VulkanContext *VulkanBackend::GetContext() { return context; }

static bool createVulkanInstance(VkApplicationInfo application_info,
                                 SDL_Window *window, VkInstance *out_instance) {
  VulkanContext *context = VulkanBackend::GetContext();

  std::vector<const char *> required_layers;
#ifndef NDEBUG
  required_layers.emplace_back("VK_LAYER_KHRONOS_validation");
  DEBUG("Required validation layers:");
  for (int i = 0; i < required_layers.size(); ++i) {
    DEBUG("%s", required_layers[i]);
  }
#endif
  if (!requiredLayersAvailable(required_layers)) {
    return false;
  }

  uint32_t required_extensions_count = 0;
  std::vector<const char *> required_extensions;
  SDL_Vulkan_GetInstanceExtensions(window, &required_extensions_count,
                                   required_extensions.data());
  required_extensions.resize(required_extensions_count);
  if (!SDL_Vulkan_GetInstanceExtensions(window, &required_extensions_count,
                                        required_extensions.data())) {
    ERROR("Failed to get SDL Vulkan extensions!");
    return false;
  }
#ifndef NDEBUG
  required_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
#ifdef PLATFORM_APPLE
  required_extensions.emplace_back(
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
  DEBUG("Required extensions:");
  for (int i = 0; i < required_extensions.size(); ++i) {
    DEBUG(required_extensions[i]);
  }

  if (!requiredExtensionsAvailable(required_extensions)) {
    return false;
  }

  VkInstanceCreateInfo instance_create_info = {};
  instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pNext = 0;
#if PLATFORM_APPLE == 1
  instance_create_info.flags |=
      VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
  instance_create_info.pApplicationInfo = &application_info;
  instance_create_info.enabledLayerCount = required_layers.size();
  instance_create_info.ppEnabledLayerNames = required_layers.data();
  instance_create_info.enabledExtensionCount = required_extensions.size();
  instance_create_info.ppEnabledExtensionNames = required_extensions.data();

  VK_CHECK(vkCreateInstance(&instance_create_info, context->allocator,
                            out_instance));

  return true;
}

static bool createVulkanSurface(SDL_Window *window, VkSurfaceKHR *out_surface) {
  VulkanContext *context = VulkanBackend::GetContext();

  if (!SDL_Vulkan_CreateSurface(window, context->instance, out_surface)) {
    ERROR("Failed to create vulkan surface.");
    return false;
  }

  return true;
}

static bool
createVulkanDebugMessanger(VkDebugUtilsMessengerEXT *out_debug_messenger) {
  VulkanContext *context = VulkanBackend::GetContext();

  uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
  debug_create_info.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_create_info.pNext = 0;
  debug_create_info.flags = 0;
  debug_create_info.messageSeverity = log_severity;
  debug_create_info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debug_create_info.pfnUserCallback = vulkanDebugCallback;
  debug_create_info.pUserData = 0;

  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          context->instance, "vkCreateDebugUtilsMessengerEXT");
  VK_CHECK(func(context->instance, &debug_create_info, context->allocator,
                out_debug_messenger));

  /* TODO: there is a lot more debugging function pointers, like
   * PFN_vkSetDebugUtilsObjectNameEXT */

  return true;
}

static bool requiredLayersAvailable(std::vector<const char *> required_layers) {
  uint32_t available_layer_count = 0;
  std::vector<VkLayerProperties> available_layers;

  VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
  available_layers.resize(available_layer_count);
  VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count,
                                              &available_layers[0]));

  for (int i = 0; i < required_layers.size(); ++i) {
    bool found = false;
    for (int j = 0; j < available_layer_count; ++j) {
      if (strcmp(required_layers[i], available_layers[j].layerName) == 0) {
        found = true;
        DEBUG("Required validation layer found: %s.", required_layers[i]);
        break;
      }
    }

    if (!found) {
      ERROR("Required validation layer is missing: %s.", required_layers[i]);
      return false;
    }
  }

  return true;
}

static bool
requiredExtensionsAvailable(std::vector<const char *> required_extensions) {
  uint32_t available_extension_count = 0;
  std::vector<VkExtensionProperties> available_extensions;

  VK_CHECK(
      vkEnumerateInstanceExtensionProperties(0, &available_extension_count, 0));
  available_extensions.resize(available_extension_count);
  VK_CHECK(vkEnumerateInstanceExtensionProperties(0, &available_extension_count,
                                                  &available_extensions[0]));

  for (int i = 0; i < required_extensions.size(); ++i) {
    bool found = false;
    for (int j = 0; j < available_extension_count; ++j) {
      if (strcmp(required_extensions[i],
                 available_extensions[j].extensionName) == 0) {
        found = true;
        DEBUG("Required exension found: %s.", required_extensions[i]);
        break;
      }
    }

    if (!found) {
      DEBUG("Required extension is missing: %s.", required_extensions[i]);
      return false;
    }
  }

  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                    VkDebugUtilsMessageTypeFlagsEXT message_types,
                    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                    void *user_data) {
  switch (message_severity) {
  default:
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
    ERROR(callback_data->pMessage);
  } break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
    WARN(callback_data->pMessage);
  } break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
    INFO(callback_data->pMessage);
  } break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
    TRACE(callback_data->pMessage);
  } break;
  }

  return VK_FALSE;
}