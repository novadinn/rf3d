cmake_minimum_required(VERSION 3.9)

project(rf3d)

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

set(INC
  ${Vulkan_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/src
)

include_directories(
  ${Vulkan_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/vendor
  ${CMAKE_SOURCE_DIR}/vendor/glad/include
)

add_definitions(
  -DGLM_FORCE_LEFT_HANDED
)

set(SRC 
  src/main.cpp 
  src/logger.cpp 
  src/renderer/renderer_frontend.cpp 
  src/renderer/gpu_utils.cpp
  src/renderer/vulkan/vulkan_backend.cpp
  src/renderer/vulkan/vulkan_device.cpp
  src/renderer/vulkan/vulkan_swapchain.cpp
  src/renderer/vulkan/vulkan_utils.cpp
  src/renderer/vulkan/vulkan_command_buffer.cpp
  src/renderer/vulkan/vulkan_render_pass.cpp
  src/renderer/vulkan/vulkan_framebuffer.cpp
  src/renderer/vulkan/vulkan_fence.cpp
  src/renderer/vulkan/vulkan_shader.cpp
  src/renderer/vulkan/vulkan_pipeline.cpp
  src/renderer/vulkan/vulkan_buffer.cpp
  src/renderer/vulkan/vulkan_texture.cpp
  src/renderer/vulkan/vulkan_attachment.cpp
  src/renderer/vulkan/vulkan_vertex_buffer.cpp
  src/renderer/vulkan/vulkan_index_buffer.cpp
  src/renderer/vulkan/vulkan_uniform_buffer.cpp
  src/renderer/vulkan/vulkan_descriptor_pools.cpp
  src/renderer/vulkan/vulkan_descriptor_layout_cache.cpp
  src/renderer/vulkan/vulkan_descriptor_builder.cpp
  src/renderer/vulkan/vulkan_descriptor_set.cpp
)

add_executable(
  ${PROJECT_NAME}
  ${SRC}
)

add_subdirectory(vendor/glm EXCLUDE_FROM_ALL)

target_link_directories(
  ${PROJECT_NAME}
  PRIVATE vendor/SPIRV-Cross
  PRIVATE /usr/local/lib
)

target_link_libraries(
  ${PROJECT_NAME}
  ${Vulkan_LIBRARIES}
  ${SDL2_LIBRARIES}
  glm
  spirv-cross-core
  spirv-cross-glsl
)

file(GLOB TEXTURES
  "assets/textures/brickwall.jpg"
  "assets/textures/metal.png"
  "assets/textures/wood.png"
  "assets/textures/grass.png"
)
foreach(TEXTURE ${TEXTURES})
  get_filename_component(FILE_NAME ${TEXTURE} NAME)
  file(COPY ${TEXTURE} DESTINATION "${PROJECT_BINARY_DIR}/assets/textures/")
endforeach(TEXTURE)

file(GLOB VK_GLSL_SOURCE_FILES
  "assets/shaders/*.frag"
  "assets/shaders/*.vert"
)
set(GLSLANG "glslangValidator")
foreach(GLSL ${VK_GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/assets/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/assets/shaders/"
    COMMAND ${GLSLANG} --target-env vulkan1.2 ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
  shaders
  DEPENDS ${SPIRV_BINARY_FILES}
)

add_custom_target(compile_shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(${PROJECT_NAME} compile_shaders)