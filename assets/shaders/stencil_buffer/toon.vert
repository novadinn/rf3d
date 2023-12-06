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

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outLightVec;

void main() 
{
	outColor = vec3(1.0, 0.0, 0.0);
	gl_Position = globalUBO.projection * globalUBO.view * instanceUBO.model * vec4(inPosition.xyz, 1.0);
	outNormal = mat3(instanceUBO.model) * inNormal;
	vec4 pos = instanceUBO.model * vec4(inPosition, 1.0);
	vec3 lPos = mat3(instanceUBO.model) * vec3(inverse(globalUBO.view)[3]);
	outLightVec = lPos - pos.xyz;
}