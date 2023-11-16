#version 450

layout(location = 0) out vec2 outTexCoords;

void main() {
  outTexCoords = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position =
      vec4(outTexCoords * vec2(2.0f, 2.0f) + vec2(-1.0f, -1.0f), 0.0f, 1.0f);
}