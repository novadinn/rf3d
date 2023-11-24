cmake_minimum_required(VERSION 3.9)

project(geometry_shader)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

add_definitions(
  -DGLM_FORCE_LEFT_HANDED
)

include_directories(
  ${RF3D_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/vendor
)

set(SRC 
  src/main.cpp
  src/../../base/camera.cpp
  src/../../base/input.cpp
)

add_executable(
  ${PROJECT_NAME}
  ${SRC}
)

target_link_directories(${PROJECT_NAME} 
  PRIVATE ${RF3D_LIB_DIR}
)
target_link_libraries(${PROJECT_NAME} rf3d ${SDL2_LIBRARIES})