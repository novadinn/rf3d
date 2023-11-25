#pragma once

#include "camera.h"
#include "input.h"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <rf3d/framework/renderer/renderer_frontend.h>
#include <stdint.h>

class Example {
public:
  Example(const char *example_name, int window_width, int window_height);
  virtual ~Example();

  virtual void EventLoop() = 0;

protected:
  void UpdateStart();
  void UpdateEnd();

  SDL_Window *window;
  RendererFrontend *frontend;
  Camera *camera;
  int width, height;

  bool running;
  uint32_t start_time_ms;
  glm::ivec2 previous_mouse;
  uint32_t last_update_time;
};