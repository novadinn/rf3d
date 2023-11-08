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

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outCameraPosition;

void main() {
  outWorldPos = vec3(instanceUBO.model * vec4(inPosition, 1.0));
  outNormal = mat3(instanceUBO.model) * inNormal;
  outCameraPosition = vec3(inverse(globalUBO.view)[3]);

  gl_Position = globalUBO.projection * globalUBO.view * vec4(outWorldPos, 1.0);
}