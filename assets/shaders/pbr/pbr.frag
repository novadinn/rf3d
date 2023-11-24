#version 450

layout(location = 0) in vec3 outWorldPos;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec3 outCameraPosition;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform WorldUBO { vec4 lights[4]; }
worldUBO;

layout(push_constant) uniform PushConsts {
  layout(offset = 0) float roughness;
  layout(offset = 4) float metallic;
  layout(offset = 8) float r;
  layout(offset = 12) float g;
  layout(offset = 16) float b;
}
material;

const float PI = 3.14159265359;

float D_GGX(float dotNH, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
  return (alpha2) / (PI * denom * denom);
}

float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;
  float GL = dotNL / (dotNL * (1.0 - k) + k);
  float GV = dotNV / (dotNV * (1.0 - k) + k);
  return GL * GV;
}

vec3 F_Schlick(float cosTheta, float metallic) {
  vec3 F0 = mix(vec3(0.04), vec3(material.r, material.g, material.b), metallic);
  vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
  return F;
}

vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness) {
  vec3 H = normalize(V + L);
  float dotNV = clamp(dot(N, V), 0.0, 1.0);
  float dotNL = clamp(dot(N, L), 0.0, 1.0);
  float dotNH = clamp(dot(N, H), 0.0, 1.0);

  vec3 lightColor = vec3(1.0);

  vec3 color = vec3(0.0);

  if (dotNL > 0.0) {
    float rroughness = max(0.05, roughness);
    float D = D_GGX(dotNH, roughness);
    float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
    vec3 F = F_Schlick(dotNV, metallic);

    vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

    color += spec * dotNL * lightColor;
  }

  return color;
}

void main() {
  vec3 N = normalize(outNormal);
  vec3 V = normalize(outCameraPosition - outWorldPos);

  vec3 Lo = vec3(0.0);

  for (int i = 0; i < worldUBO.lights.length(); i++) {
    vec3 L = normalize(worldUBO.lights[i].xyz - outWorldPos);
    Lo += BRDF(L, V, N, material.metallic, material.roughness);
  }

  vec3 color = vec3(material.r, material.g, material.b) * 0.02;
  color += Lo;

  color = pow(color, vec3(0.4545));

  outColor = vec4(color, 1.0);
}