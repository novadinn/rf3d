#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;

layout(location = 0) out vec2 outTexCoords;

void main() {
  inPosition;
  inTexCoords;
  outTexCoords = inTexCoords;
  gl_Position = vec4(inPosition, 1.0);
}