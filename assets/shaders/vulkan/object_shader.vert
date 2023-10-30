#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform uniform_buffer_object {
  mat4 model;
  mat4 view;
  mat4 projection;
  mat4 _pad0; /* some nvidia cards requires alignment of 256 bytes */
}
ubo;

void main() {
  gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
}