#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

class Camera {
public:
  void Create(float start_fov, float start_aspect_ratio, float start_near,
              float start_far);

  void Pan(const glm::vec2 &delta);
  void Rotate(const glm::vec2 &delta);
  void Zoom(float delta);

  void SetViewportSize(float width, float height);

  glm::mat4 GetProjectionMatrix();
  glm::mat4 GetViewMatrix();
  glm::vec3 GetUp();
  glm::vec3 GetRight();
  glm::vec3 GetForward();
  glm::quat GetOrientation();
  glm::vec2 GetPanSpeed();
  glm::vec3 GetPosition();
  glm::vec3 GetFocalPoint();
  float GetRotationSpeed();
  float GetZoomSpeed();

  glm::vec3 ScreenToWorldDirection(const glm::vec2 &point);

private:
  float fov = 45.0f, near = 0.1f, far = 1000.0f;
  glm::vec3 position;
  float pitch = 0.0f, yaw = 0.0f;
  glm::vec3 focal_point = {0, 0, 0};
  float aspect_ratio = 1.778f;
  float distance = 10.0f;
  float viewport_width = 800, viewport_height = 600;
};