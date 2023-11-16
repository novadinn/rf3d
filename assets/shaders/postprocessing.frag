#version 450

layout(location = 0) in vec2 outTexCoords;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D samplerTexture;

vec3 BlurColor(float offset, sampler2D textureSampler, vec2 texCoords) {
  vec2 offsets[9] = vec2[](vec2(-offset, offset),  // top-left
                           vec2(0.0f, offset),     // top-center
                           vec2(offset, offset),   // top-right
                           vec2(-offset, 0.0f),    // center-left
                           vec2(0.0f, 0.0f),       // center-center
                           vec2(offset, 0.0f),     // center-right
                           vec2(-offset, -offset), // bottom-left
                           vec2(0.0f, -offset),    // bottom-center
                           vec2(offset, -offset)   // bottom-right
  );
  /* blur effect */
  float kernel[9] = float[](1.0 / 16, 2.0 / 16, 1.0 / 16, 2.0 / 16, 4.0 / 16,
                            2.0 / 16, 1.0 / 16, 2.0 / 16, 1.0 / 16);

  vec3 sampleTex[9];
  for (int i = 0; i < 9; i++) {
    sampleTex[i] = vec3(texture(textureSampler, texCoords.st + offsets[i]));
  }

  vec3 col = vec3(0.0);
  for (int i = 0; i < 9; i++)
    col += sampleTex[i] * kernel[i];

  return col;
}

void main() {
  outColor = texture(samplerTexture, outTexCoords);

  const float offset = 1.0 / 300.0;

  outColor = vec4(BlurColor(offset, samplerTexture, outTexCoords), 1.0);
}