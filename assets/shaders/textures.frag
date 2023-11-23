#version 450

layout(location = 0) in vec2 outTexCoords;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D objectTexture;

void main() {
  vec3 ambientColor = vec3(0.1);
  outColor = vec4(texture(objectTexture, outTexCoords).xyz + ambientColor, 1.0);
}