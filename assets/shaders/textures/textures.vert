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

layout(location = 0) out vec2 outTexCoords;

void main() {
  vec3 worldPos = vec3(instanceUBO.model * vec4(inPosition, 1.0));
  /* TODO: seems like we should use attributes in the order specified in layout.
   * later, we need to sort them in the vulkan_shader */
  inNormal;
  outTexCoords = inTexCoords;

  gl_Position = globalUBO.projection * globalUBO.view * vec4(worldPos, 1.0);
}