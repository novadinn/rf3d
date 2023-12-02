#version 450

layout(location = 0) in float outHeight;

layout(location = 0) out vec4 outColor;

void main() {
  float h = (outHeight + 16.0) / 64.0;
  outColor = vec4(h, h, h, 1.0);
}