#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(set = 0, binding = 0) uniform GlobalUBO {
  mat4 view;
  mat4 projection;
}
globalUBO;
layout(set = 1, binding = 0) uniform InstanceUBO { mat4 model; }
instanceUBO;

layout(location = 0) out vec3 outNormal;

void main() {
  outNormal = mat3(instanceUBO.model) * inNormal;

  gl_Position = vec4(inPosition, 1.0);
}