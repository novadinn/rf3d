#version 450

layout(location = 0) in vec3 outWorldPosition;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec2 outTexCoords;
layout(location = 3) in vec3 outTangent;

layout(location = 0) out vec4 outPositionColor;
layout(location = 1) out vec4 outNormalColor;
layout(location = 2) out vec4 outAlbedoColor;

layout(set = 2, binding = 0) uniform sampler2D diffuseMap;
layout(set = 2, binding = 1) uniform sampler2D specularMap;
layout(set = 2, binding = 2) uniform sampler2D normalMap;

void main() {
  vec3 N = normalize(outNormal);
  vec3 T = normalize(outTangent);
  vec3 B = cross(N, T);
  mat3 TBN = mat3(T, B, N);
  vec3 tnorm =
      TBN * normalize(texture(normalMap, outTexCoords).xyz * 2.0 - vec3(1.0));

  outPositionColor = vec4(outWorldPosition, 1.0);
  outNormalColor = vec4(tnorm, 1.0);
  outAlbedoColor = texture(diffuseMap, outTexCoords);
}