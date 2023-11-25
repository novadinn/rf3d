#include <iostream>

#include "../base/camera.h"
#include "../base/input.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rf3d/framework/logger.h>
#include <rf3d/framework/renderer/renderer_frontend.h>

struct PushConsts {
  float roughness;
  float metallic;
  float r;
  float g;
  float b;
};

struct MeshParams {
  glm::vec3 position;
  PushConsts push_constants;
};

struct GlobalUBO {
  glm::mat4 view;
  glm::mat4 projection;
};

struct WorldUBO {
  glm::vec4 lights[4];
};

struct InstanceUBO {
  glm::mat4 model;
};

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    FATAL("Couldn't initialize SLD");
    exit(1);
  }

  uint32_t width = 800;
  uint32_t height = 600;

  SDL_Window *window =
      SDL_CreateWindow("RF3D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       width, height, SDL_WINDOW_VULKAN);
  RendererFrontend *frontend = new RendererFrontend();
  if (!frontend->Initialize(window, RendererBackendType::RBT_VULKAN)) {
    exit(1);
  }

  GPUVertexBuffer *vertex_buffer = frontend->VertexBufferAllocate();
  GPUShader *shader = frontend->ShaderAllocate();
  GPURenderPass *window_render_pass = frontend->GetWindowRenderPass();

  std::vector<MeshParams> meshes(3);

  meshes[0].position = glm::vec3(0, 0, 0.0f);
  meshes[0].push_constants =
      PushConsts{0.1f, 1.0f, 0.672411f, 0.637331f, 0.585456f};
  meshes[1].position = glm::vec3(2, 0, 0.0f);
  meshes[1].push_constants = PushConsts{0.8f, 0.2f, 1, 0, 0};
  meshes[2].position = glm::vec3(-2, 0, 0.0f);
  meshes[2].push_constants = PushConsts{0.5f, 0.5f, 0, 1, 0};

  std::vector<float> vertices = {
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.5f,  -0.5f,
      -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,
      0.0f,  -1.0f, 1.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
      1.0f,  1.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.5f,  -0.5f,
      0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,
      0.0f,  1.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
      1.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,
      -0.5f, -1.0f, 0.0f,  0.0f,  1.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f,
      0.0f,  0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
      0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,
      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,
      -0.5f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,
      0.0f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
      0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.5f,  -0.5f,
      -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,
      -1.0f, 0.0f,  1.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
      1.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.0f,
      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,
      -0.5f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,
      1.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f};

  if (!vertex_buffer->Create(vertices.size() * sizeof(vertices[0]))) {
    ERROR("Failed to create a vertex buffer!");
    exit(1);
  }
  vertex_buffer->LoadData(0, vertices.size() * sizeof(vertices[0]),
                          vertices.data());

  std::vector<GPUShaderStageConfig> stage_configs;
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/pbr.vert.spv"});
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/pbr.frag.spv"});
  if (!shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                      GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                          GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                      window_render_pass, width, height)) {
    FATAL("Failed to create a shader. Aborting...");
    exit(1);
  }

  GPUUniformBuffer *global_uniform = frontend->UniformBufferAllocate();
  GPUUniformBuffer *world_uniform = frontend->UniformBufferAllocate();
  GPUUniformBuffer *instance_uniform = frontend->UniformBufferAllocate();

  global_uniform->Create(sizeof(GlobalUBO));
  world_uniform->Create(sizeof(WorldUBO));
  instance_uniform->Create(sizeof(InstanceUBO), meshes.size());

  GPUDescriptorSet *global_descriptor_set = frontend->DescriptorSetAllocate();
  GPUDescriptorSet *world_descriptor_set = frontend->DescriptorSetAllocate();
  GPUDescriptorSet *instance_descriptor_set = frontend->DescriptorSetAllocate();

  std::vector<GPUDescriptorBinding> bindings;

  bindings.emplace_back(GPUDescriptorBinding{
      0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, global_uniform});
  global_descriptor_set->Create(bindings);
  bindings.clear();

  bindings.emplace_back(GPUDescriptorBinding{
      0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, world_uniform});
  world_descriptor_set->Create(bindings);
  bindings.clear();

  bindings.emplace_back(GPUDescriptorBinding{
      0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, instance_uniform});
  instance_descriptor_set->Create(bindings);
  bindings.clear();

  Camera *camera = new Camera();
  camera->Create(45, width / height, 0.1f, 1000.0f);
  camera->SetViewportSize(width, height);

  glm::ivec2 previous_mouse = {0, 0};
  uint32_t last_update_time = SDL_GetTicks();

  bool running = true;
  while (running) {
    uint32_t start_time_ms = SDL_GetTicks();
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
      camera->Rotate(mouse_delta);
    }
    if (wheel_movement.y != 0) {
      camera->Zoom(delta_time * wheel_movement.y);
    }

    if (frontend->BeginFrame()) {
      window_render_pass->Begin(frontend->GetCurrentWindowRenderTarget());

      glm::vec3 camera_position = glm::vec3(0, 0, 0.0f);
      GlobalUBO global_ubo = {};
      global_ubo.view = camera->GetViewMatrix();
      global_ubo.projection = camera->GetProjectionMatrix();
      global_uniform->LoadData(0, global_uniform->GetSize(), &global_ubo);

      shader->Bind();
      vertex_buffer->Bind(0);
      shader->BindUniformBuffer(global_descriptor_set, 0, 0);

      WorldUBO world_ubo = {};
      const float p = 5.0f;
      world_ubo.lights[0] = glm::vec4(-p * 0.8f, -p * 0.8f, p * 0.8f, 5.0f);
      world_ubo.lights[1] = glm::vec4(-p * 2, p * 2, p * 2, 5.0f);
      world_ubo.lights[2] = glm::vec4(p * 0.2f, -p * 0.2f, p * 0.2f, 5.0f);
      world_ubo.lights[3] = glm::vec4(p, p, p, 5.0f);

      world_uniform->LoadData(0, world_uniform->GetSize(), &world_ubo);
      shader->BindUniformBuffer(world_descriptor_set, 0, 1);

      for (int i = 0; i < meshes.size(); ++i) {
        std::vector<InstanceUBO> instance_ubos;
        for (int j = 0; j < meshes.size(); ++j) {
          InstanceUBO instance_ubo = {};
          instance_ubo.model = glm::mat4(1.0f);
          instance_ubo.model =
              glm::translate(instance_ubo.model, meshes[j].position);

          instance_ubos.emplace_back(instance_ubo);
        }

        instance_uniform->LoadData(0, instance_uniform->GetSize(),
                                   instance_ubos.data());
        shader->BindUniformBuffer(instance_descriptor_set,
                                  i * instance_uniform->GetDynamicAlignment(),
                                  2);

        shader->PushConstant(&meshes[i].push_constants, sizeof(PushConsts), 0,
                             GPU_SHADER_STAGE_TYPE_FRAGMENT);

        frontend->Draw(vertices.size() / 8);
      }

      window_render_pass->End();

      frontend->EndFrame();
    }

    const uint32_t ms_per_frame = 1000 / 120;
    const uint32_t elapsed_time_ms = SDL_GetTicks() - start_time_ms;
    if (elapsed_time_ms < ms_per_frame) {
      SDL_Delay(ms_per_frame - elapsed_time_ms);
    }

    Input::GetMousePosition(&previous_mouse.x, &previous_mouse.y);
  }

  delete camera;

  instance_uniform->Destroy();
  delete instance_uniform;
  world_uniform->Destroy();
  delete world_uniform;
  global_uniform->Destroy();
  delete global_uniform;
  shader->Destroy();
  delete shader;
  vertex_buffer->Destroy();
  delete vertex_buffer;

  frontend->Shutdown();
  delete frontend;

  SDL_DestroyWindow(window);

  return 0;
}