#include "camera.h"

#include <algorithm>

void Camera::Create(float start_fov, float start_aspect_ratio, float start_near,
                    float start_far) {
  fov = start_fov;
  aspect_ratio = start_aspect_ratio;
  near = start_near;
  far = start_far;
}

void Camera::Pan(const glm::vec2 &delta) {
  glm::vec2 speed = GetPanSpeed();
  focal_point += -GetRight() * delta.x * speed.x * distance;
  focal_point += -GetUp() * delta.y * speed.y * distance;
}

void Camera::Rotate(const glm::vec2 &delta) {
  float yaw_sign = GetUp().y < 0 ? 1.0f : -1.0f;
  yaw += yaw_sign * delta.x * GetRotationSpeed();
  pitch += delta.y * GetRotationSpeed();
}

void Camera::Zoom(float delta) {
  distance -= delta * GetZoomSpeed();
  if (distance < 1.0f) {
    focal_point += GetForward();
    distance = 1.0f;
  }
}

void Camera::SetViewportSize(float width, float height) {
  viewport_width = width;
  viewport_height = height;
}

glm::mat4 Camera::GetProjectionMatrix() {
  aspect_ratio = viewport_width / viewport_height;
  return glm::perspective(glm::radians(fov), aspect_ratio, near, far);
}

glm::mat4 Camera::GetViewMatrix() {
  position = focal_point - GetForward() * distance;

  glm::quat orientation = GetOrientation();
  glm::mat4 view =
      glm::translate(glm::mat4(1.0f), position) * glm::toMat4(orientation);
  view = glm::inverse(view);

  return view;
}

glm::vec3 Camera::GetUp() {
  return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::GetRight() {
  return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Camera::GetForward() {
  return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::quat Camera::GetOrientation() {
  return glm::quat(glm::vec3(-pitch, -yaw, 0.0f));
}

glm::vec3 Camera::GetPosition() { return position; }

glm::vec3 Camera::GetFocalPoint() { return focal_point; }

glm::vec2 Camera::GetPanSpeed() {
  float x = std::min(viewport_width / 1000.0f, 2.4f);
  float x_factor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

  float y = std::min(viewport_height / 1000.0f, 2.4f);
  float y_factor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

  return {x_factor, y_factor};
}

float Camera::GetRotationSpeed() { return 0.8f; }

float Camera::GetZoomSpeed() {
  float dst = distance * 0.8f;
  dst = std::max(dst, 0.0f);
  float speed = dst * dst;
  speed = std::min(speed, 100.0f);

  return speed;
}

glm::vec3 Camera::ScreenToWorldDirection(const glm::vec2 &point) {
  glm::vec3 ndc = {(2.0f * point.x) / viewport_width - 1.0f,
                   (2.0f * point.y) / viewport_height - 1.0f, 1.0f};

  glm::vec4 clip = glm::vec4(ndc.x, ndc.y, -1.0f, 1.0f);

  glm::vec4 eye = glm::inverse(GetProjectionMatrix()) * clip;
  eye = glm::vec4(eye.x, eye.y, -1.0f, 0.0f);

  glm::vec3 world = glm::vec3(glm::inverse(GetViewMatrix()) * eye);
  world = glm::normalize(world);

  return world;
}