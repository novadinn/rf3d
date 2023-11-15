#version 450

layout(location = 0) in vec3 outPosition;
layout(location = 1) in vec3 outVertexColor;
layout(location = 2) in vec3 outCameraPosition;

layout(location = 0) out vec4 outColor;

float InverseLerp(float from, float to, float t) {
  return (t - from) / (to - from);
}

void main() {
  const float fadeOffset = 30.0;
  /* TODO: constant, but we can retrieve that from the projection matrix */
  const float far = 100.0f;

  float distanceToCamera = distance(outCameraPosition, outPosition);
  float t = InverseLerp(far - fadeOffset, far, distanceToCamera);

  outColor = vec4(outVertexColor, 1.0);
  outColor.a = mix(0, 1, 1 - t);
}