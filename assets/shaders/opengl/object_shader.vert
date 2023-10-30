#version 410 core

layout(location = 0) in vec3 inPosition;

layout(std140) uniform uniform_buffer_object {
  mat4 model;
  mat4 view;
  mat4 projection;
  mat4 _pad0; /* some nvidia cards requires alignment of 256 bytes */
};

void main() { gl_Position = projection * view * model * vec4(inPosition, 1.0); }