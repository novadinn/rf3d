#version 410 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(std140) uniform uniform_buffer_object {
  mat4 model;
  mat4 view;
  mat4 projection;
  mat4 _pad0; /* some nvidia cards requires alignment of 256 bytes */
};

out vec3 outWorldPos;
out vec3 outNormal;
out vec3 outCameraPosition;

void main() {
  outWorldPos = vec3(model * vec4(inPosition, 1.0));
  outNormal = mat3(model) * inNormal;
  outCameraPosition = vec3(inverse(view)[3]);

  gl_Position = projection * view * vec4(outWorldPos, 1.0);
}