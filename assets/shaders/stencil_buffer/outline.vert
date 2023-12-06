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

void main() {
  const float outlineWidth = 0.05;
	vec4 position = vec4(inPosition.xyz + inNormal * outlineWidth, 1.0);
	gl_Position = globalUBO.projection * globalUBO.view * instanceUBO.model * position;
}
