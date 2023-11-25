cmake_minimum_required(VERSION 3.9)

project(rf3d)

set(CMAKE_CXX_STANDARD 20)

if(NOT DEFINED RF3D_INCLUDE_DIR) 
  message(FATAL_ERROR "RF3D include directory (RF3D_INCLUDE_DIR) is not provided. Aborting..." )
endif()
if(NOT DEFINED RF3D_LIB_DIR)
  message(FATAL_ERROR "RF3D library directory (RF3D_LIB_DIR) is not provided. Aborting..." )
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

add_subdirectory(${CMAKE_SOURCE_DIR}/framework)
add_subdirectory(${CMAKE_SOURCE_DIR}/examples)

file(GLOB_RECURSE TEXTURE_ASSETS
  "assets/textures/*.jpg"
  "assets/textures/*.png"
)
foreach(TEXTURE ${TEXTURE_ASSETS})
  get_filename_component(FILE_NAME ${TEXTURE} NAME)
  file(COPY ${TEXTURE} DESTINATION "${PROJECT_BINARY_DIR}/bin/assets/textures/")
endforeach(TEXTURE)

file(GLOB_RECURSE VK_GLSL_SOURCE_FILES
  "assets/shaders/*.frag"
  "assets/shaders/*.geom"
  "assets/shaders/*.vert"
)
set(GLSLANG "glslangValidator")
foreach(GLSL ${VK_GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/bin/assets/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/bin/assets/shaders/"
    COMMAND ${GLSLANG} --target-env vulkan1.2 ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
  shaders 
  DEPENDS ${SPIRV_BINARY_FILES}
)
add_dependencies(${PROJECT_NAME} shaders)