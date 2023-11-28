#version 450

layout(location = 0) in vec3 outWorldPosition;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec2 outTexCoords;
layout(location = 3) in vec3 outTangent;
layout(location = 4) in vec3 outBitangent;
layout(location = 5) in vec3 outCameraPosition;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D diffuseMap;
layout(set = 2, binding = 1) uniform sampler2D specularMap;
layout(set = 2, binding = 2) uniform sampler2D normalMap;

vec4 CalculateDirectionalLight(vec3 lightDirection, vec3 lightColor,
                               vec3 normal, vec3 viewDirection);

void main() {
  vec3 normal = outNormal;
  vec3 tangent = outTangent;
  tangent = (tangent - dot(tangent, normal) * normal);
  vec3 bitangent = cross(outNormal, outTangent);
  mat3 TBN = mat3(tangent, bitangent, normal);
  vec3 localNormal = 2.0 * texture(normalMap, outTexCoords).rgb - 1.0;
  normal = normalize(TBN * localNormal);

  vec3 viewDirection = normalize(outCameraPosition - outWorldPosition);
  outColor =
      CalculateDirectionalLight(vec3(1.0), vec3(1.0), normal, viewDirection);
}

vec4 CalculateDirectionalLight(vec3 lightDirection, vec3 lightColor,
                               vec3 normal, vec3 viewDirection) {
  float diffuseFactor = max(dot(normal, -lightDirection.xyz), 0.0);

  vec3 H = normalize(viewDirection - lightDirection.xyz);
  float specularFactor = pow(max(dot(H, normal), 0.0), 0.2);

  vec4 diffuseSamp = texture(diffuseMap, outTexCoords);
  vec4 ambient = vec4(vec3(0.2, 0.2, 0.2), diffuseSamp.a);
  vec4 diffuse = vec4(vec3(lightColor * diffuseFactor), diffuseSamp.a);
  vec4 specular = vec4(vec3(lightColor * specularFactor), diffuseSamp.a);

  diffuse *= diffuseSamp;
  ambient *= diffuseSamp;
  specular *= vec4(texture(specularMap, outTexCoords).rgb, diffuse.a);

  return (ambient + diffuse + specular);
}