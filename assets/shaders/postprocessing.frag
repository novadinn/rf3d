#version 450

layout(location = 0) noperspective in vec2 outTexCoords;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D samplerTexture;

void main() { outColor = texture(samplerTexture, outTexCoords); }