cmake_minimum_required(VERSION 3.9)

include(GNUInstallDirs)

project(rf3d VERSION 1.0.0 DESCRIPTION "Rendering Library")

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)

if (DEFINED VULKAN_SDK_PATH)
  set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
  set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
  set(Vulkan_FOUND "True")
else()
  find_package(Vulkan REQUIRED)
  message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else()
    message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif()

include_directories(
  ${Vulkan_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/../vendor
  ${CMAKE_SOURCE_DIR}/../vendor/glad/include
  ${CMAKE_SOURCE_DIR}/../vendor/VulkanMemoryAllocator/include
  ${CMAKE_SOURCE_DIR}/../vendor/glm
)

add_definitions(
  -DGLM_FORCE_LEFT_HANDED
)

set(SRC 
  logger.cpp 
  renderer/renderer_frontend.cpp 
  renderer/gpu_utils.cpp
  renderer/vulkan/vulkan_backend.cpp
  renderer/vulkan/vulkan_device.cpp
  renderer/vulkan/vulkan_swapchain.cpp
  renderer/vulkan/vulkan_utils.cpp
  renderer/vulkan/vulkan_command_buffer.cpp
  renderer/vulkan/vulkan_render_pass.cpp
  renderer/vulkan/vulkan_framebuffer.cpp
  renderer/vulkan/vulkan_fence.cpp
  renderer/vulkan/vulkan_shader.cpp
  renderer/vulkan/vulkan_pipeline.cpp
  renderer/vulkan/vulkan_buffer.cpp
  renderer/vulkan/vulkan_texture.cpp
  renderer/vulkan/vulkan_attachment.cpp
  renderer/vulkan/vulkan_vertex_buffer.cpp
  renderer/vulkan/vulkan_index_buffer.cpp
  renderer/vulkan/vulkan_uniform_buffer.cpp
  renderer/vulkan/vulkan_descriptor_pools.cpp
  renderer/vulkan/vulkan_descriptor_layout_cache.cpp
  renderer/vulkan/vulkan_descriptor_builder.cpp
  renderer/vulkan/vulkan_descriptor_set.cpp
  renderer/vulkan/vulkan_debug_marker.cpp
)

add_library(
  ${PROJECT_NAME} SHARED
  ${SRC}
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${RF3D_LIB_DIR})
install(DIRECTORY "${CMAKE_SOURCE_DIR}/"
    DESTINATION ${RF3D_INCLUDE_DIR}/rf3d
    FILES_MATCHING
    PATTERN "*.h"
)

target_link_directories(
  ${PROJECT_NAME}
  PRIVATE ../vendor/SPIRV-Cross
  PRIVATE /usr/local/lib
)

target_link_libraries(
  ${PROJECT_NAME}
  ${Vulkan_LIBRARIES}
  ${SDL2_LIBRARIES}
  spirv-cross-core
  spirv-cross-glsl
)