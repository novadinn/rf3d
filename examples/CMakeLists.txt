add_definitions(
  -DGLM_FORCE_LEFT_HANDED
)

function(buildExample EXAMPLE_NAME)
	SET(EXAMPLE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME})
	file(GLOB SOURCE *.cpp ${BASE_HEADERS} ${EXAMPLE_FOLDER}/*.cpp)
  file(GLOB ADD_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/base/camera.cpp ${CMAKE_CURRENT_SOURCE_DIR}/base/input.cpp)
  SET(SOURCE ${SOURCE} ${ADD_SOURCE})
	SET(MAIN_CPP ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.cpp)

  add_executable(${EXAMPLE_NAME} ${MAIN_CPP} ${SOURCE})
  include_directories(${RF3D_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/vendor)
  target_link_directories(${EXAMPLE_NAME} PRIVATE ${RF3D_LIB_DIR})
  target_link_libraries(${EXAMPLE_NAME} rf3d ${SDL2_LIBRARIES})
endfunction(buildExample)

set(EXAMPLES
	textures
  pbr
  skybox
  fxaa
  geometry_shader
)

function(buildExamples)
	foreach(EXAMPLE ${EXAMPLES})
		buildExample(${EXAMPLE})
	endforeach(EXAMPLE)
endfunction(buildExamples)

buildExamples()
