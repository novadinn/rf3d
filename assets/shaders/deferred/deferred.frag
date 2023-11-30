#version 450

struct Light {
  vec4 position;
  vec3 color;
  float radius;
};

layout(set = 0, binding = 0) uniform WorldUBO {
  Light lights[6];
  vec4 viewPos;
}
worldUBO;

layout(set = 1, binding = 0) uniform sampler2D samplerPosition;
layout(set = 1, binding = 1) uniform sampler2D samplerNormal;
layout(set = 1, binding = 2) uniform sampler2D samplerAlbedo;

layout(location = 0) in vec2 outTexCoords;

layout(location = 0) out vec4 outColor;

void main() {
  vec3 fragPos = texture(samplerPosition, outTexCoords).rgb;
  vec3 normal = texture(samplerNormal, outTexCoords).rgb;
  vec4 albedo = texture(samplerAlbedo, outTexCoords);

  vec3 color = albedo.rgb * 0.2;

  for (int i = 0; i < worldUBO.lights.length(); ++i) {
    vec3 L = worldUBO.lights[i].position.xyz - fragPos;
    float dist = length(L);

    vec3 V = worldUBO.viewPos.xyz - fragPos;
    V = normalize(V);

    {
      L = normalize(L);

      float atten = worldUBO.lights[i].radius / (pow(dist, 2.0) + 1.0);

      vec3 N = normalize(normal);
      float NdotL = max(0.0, dot(N, L));
      vec3 diff = worldUBO.lights[i].color * albedo.rgb * NdotL * atten;

      vec3 R = reflect(-L, N);
      float NdotR = max(0.0, dot(R, V));
      vec3 spec =
          worldUBO.lights[i].color * albedo.a * pow(NdotR, 16.0) * atten;

      color += diff + spec;
    }
  }

  outColor = vec4(color, 1.0);
}