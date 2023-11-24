#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform GlobalUBO {
  mat4 view;
  mat4 projection;
}
globalUBO;

layout(location = 0) out vec3 outTexCoords;

void main() {
  outTexCoords = inPosition;
  /* remove translation from view matrix */
  gl_Position =
      globalUBO.projection * mat4(mat3(globalUBO.view)) * vec4(inPosition, 1.0);
}
