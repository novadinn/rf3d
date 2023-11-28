#include "example.h"

Example::Example(const char *example_name, int window_width,
                 int window_height) {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    exit(1);
  }

  width = window_width;
  height = window_height;
  window = SDL_CreateWindow(example_name, SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, width, height,
                            SDL_WINDOW_VULKAN);

  frontend = new RendererFrontend();
  if (!frontend->Initialize(window, RendererBackendType::RBT_VULKAN)) {
    exit(1);
  }

  camera = new Camera();
  camera->Create(45, width / height, 0.1f, 100000.0f);
  camera->SetViewportSize(width, height);

  running = true;
  glm::ivec2 previous_mouse = {0, 0};
  uint32_t last_update_time = SDL_GetTicks();
}

Example::~Example() {
  delete camera;

  frontend->Shutdown();
  delete frontend;

  SDL_DestroyWindow(window);
}

void Example::UpdateStart() {
  start_time_ms = SDL_GetTicks();
  Input::Begin();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_KEYDOWN: {
      if (!event.key.repeat) {
        Input::KeyDownEvent(event);
      }
    } break;
    case SDL_KEYUP: {
      Input::KeyUpEvent(event);
    } break;
    case SDL_MOUSEBUTTONDOWN: {
      Input::MouseButtonDownEvent(event);
    } break;
    case SDL_MOUSEBUTTONUP: {
      Input::MouseButtonUpEvent(event);
    } break;
    case SDL_MOUSEWHEEL: {
      Input::WheelEvent(event);
    } break;
    case SDL_WINDOWEVENT: {
      if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
        running = false;
      } else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);

        frontend->Resize((uint32_t)width, (uint32_t)height);
      }
    } break;
    case SDL_QUIT: {
      running = false;
    } break;
    }
  }

  float delta_time = 0.01f;
  glm::ivec2 current_mouse;
  Input::GetMousePosition(&current_mouse.x, &current_mouse.y);
  glm::vec2 mouse_delta = current_mouse - previous_mouse;
  mouse_delta *= delta_time;

  glm::ivec2 wheel_movement;
  Input::GetWheelMovement(&wheel_movement.x, &wheel_movement.y);

  if (Input::WasMouseButtonHeld(SDL_BUTTON_MIDDLE)) {
    if (Input::WasKeyHeld(SDLK_LSHIFT)) {
      camera->Pan(mouse_delta);
    } else {
      camera->Rotate(mouse_delta);
    }
  }
  if (wheel_movement.y != 0) {
    camera->Zoom(delta_time * wheel_movement.y);
  }
}

void Example::UpdateEnd() {
  const uint32_t ms_per_frame = 1000 / 120;
  const uint32_t elapsed_time_ms = SDL_GetTicks() - start_time_ms;
  if (elapsed_time_ms < ms_per_frame) {
    SDL_Delay(ms_per_frame - elapsed_time_ms);
  }

  Input::GetMousePosition(&previous_mouse.x, &previous_mouse.y);
}