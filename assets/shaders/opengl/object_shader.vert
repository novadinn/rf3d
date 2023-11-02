#version 410 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(std140) uniform uniform_buffer_object {
  mat4 model;
  mat4 view;
  mat4 projection;
  mat4 _pad0; /* some nvidia cards requires alignment of 256 bytes */
};

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outCameraPosition;
layout(location = 3) out vec2 outTexCoords;

void main() {
  outWorldPos = vec3(model * vec4(inPosition, 1.0));
  outNormal = mat3(model) * inNormal;
  outCameraPosition = vec3(inverse(view)[3]);
  outTexCoords = inTexCoords;

  gl_Position = projection * view * vec4(outWorldPos, 1.0);
}