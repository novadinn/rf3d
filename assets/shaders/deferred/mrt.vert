#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(set = 0, binding = 0) uniform GlobalUBO {
  mat4 view;
  mat4 projection;
}
globalUBO;
layout(set = 1, binding = 0) uniform InstanceUBO { mat4 model; }
instanceUBO;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoords;
layout(location = 3) out vec3 outTangent;

void main() {
  outWorldPosition = vec3(instanceUBO.model * vec4(inPosition, 1.0));
  mat3 mNormal = transpose(inverse(mat3(instanceUBO.model)));
  outNormal = mNormal * normalize(inNormal);
  outTexCoords = inTexCoords;
  outTangent = mNormal * normalize(inTangent);

  gl_Position =
      globalUBO.projection * globalUBO.view * vec4(outWorldPosition, 1.0);
}