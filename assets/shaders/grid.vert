#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(set = 0, binding = 0) uniform GlobalUBO {
  mat4 view;
  mat4 projection;
}
globalUBO;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outVertexColor;
layout(location = 2) out vec3 outCameraPosition;

void main() {
  outPosition = inPosition;
  outVertexColor = inColor;
  outCameraPosition = vec3(inverse(globalUBO.view)[3]);

  gl_Position = globalUBO.projection * globalUBO.view * vec4(inPosition, 1.0);
}