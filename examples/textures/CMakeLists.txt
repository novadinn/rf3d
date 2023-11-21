cmake_minimum_required(VERSION 3.9)

project(textures)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

include_directories(
  ${CMAKE_SOURCE_DIR}/src
  /usr/local/include/rf3d
  ${SDL2_INCLUDE_DIRS}
)

set(SRC 
  src/main.cpp
  src/../../base/camera.cpp
)

add_executable(
  ${PROJECT_NAME}
  ${SRC}
)

target_link_libraries(${PROJECT_NAME} rf3d ${SDL2_LIBRARIES})