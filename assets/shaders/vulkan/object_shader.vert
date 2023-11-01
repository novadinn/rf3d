#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(set = 0, binding = 0) uniform uniform_buffer_object {
  mat4 model;
  mat4 view;
  mat4 projection;
  mat4 _pad0; /* some nvidia cards requires alignment of 256 bytes */
}
ubo;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outCameraPosition;

void main() {
  outWorldPos = vec3(ubo.model * vec4(inPosition, 1.0));
  outNormal = mat3(ubo.model) * inNormal;
  outCameraPosition = vec3(inverse(ubo.view)[3]);

  gl_Position = ubo.projection * ubo.view * vec4(outWorldPos, 1.0);
}