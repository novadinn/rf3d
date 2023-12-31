#version 450

layout(location = 0) in vec3 outWorldPos;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec3 outCameraPosition;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform samplerCube samplerCubeMap;

void main() {
  vec3 I = normalize(outWorldPos - outCameraPosition);
  vec3 R = reflect(I, normalize(outNormal));
  outColor = vec4(texture(samplerCubeMap, R).rgb, 1.0);
}