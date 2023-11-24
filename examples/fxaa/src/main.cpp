#include <iostream>

#include "../../base/camera.h"
#include "../../base/input.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rf3d/framework/logger.h>
#include <rf3d/framework/renderer/renderer_frontend.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

struct MeshParams {
  GPUDescriptorSet *texture_descriptor_set;
  GPUTexture *texture;
  glm::vec3 position;
};

struct GlobalUBO {
  glm::mat4 view;
  glm::mat4 projection;
};

struct InstanceUBO {
  glm::mat4 model;
};

void LoadTexture(GPUTexture *texture, const char *path) {
  int texture_width, texture_height, texture_num_channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path, &texture_width, &texture_height,
                                  &texture_num_channels, STBI_rgb_alpha);
  if (!data) {
    FATAL("Failed to load image!");
    return;
  }

  texture->Create(GPU_FORMAT_RGBA8, GPU_TEXTURE_TYPE_2D, texture_width,
                  texture_height);
  texture->WriteData(data, 0);

  stbi_set_flip_vertically_on_load(false);
  stbi_image_free(data);
}

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    FATAL("Couldn't initialize SLD");
    exit(1);
  }

  uint32_t width = 800;
  uint32_t height = 600;

  SDL_Window *window =
      SDL_CreateWindow("RF3D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       800, 600, SDL_WINDOW_VULKAN);
  RendererFrontend *frontend = new RendererFrontend();
  if (!frontend->Initialize(window, RendererBackendType::RBT_VULKAN)) {
    exit(1);
  }

  GPUVertexBuffer *vertex_buffer = frontend->VertexBufferAllocate();
  GPUShader *shader = frontend->ShaderAllocate();
  GPURenderPass *window_render_pass = frontend->GetWindowRenderPass();

  std::vector<MeshParams> meshes(3);
  for (int i = 0; i < meshes.size(); ++i) {
    meshes[i].texture = frontend->TextureAllocate();
  }

  LoadTexture(meshes[0].texture, "../../assets/textures/metal.png");
  meshes[0].position = glm::vec3(0, 0, 5.0f);
  LoadTexture(meshes[1].texture, "../../assets/textures/wood.png");
  meshes[1].position = glm::vec3(2, 0, 5.0f);
  LoadTexture(meshes[2].texture, "../../assets/textures/brickwall.jpg");
  meshes[2].position = glm::vec3(-2, 0, 5.0f);

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

  GPUAttachment *offscreen_color_attachment = frontend->AttachmentAllocate();
  offscreen_color_attachment->Create(GPU_FORMAT_DEVICE_COLOR_OPTIMAL,
                                     GPU_ATTACHMENT_USAGE_COLOR_ATTACHMENT,
                                     width, height);
  GPUAttachment *offscreen_depth_attachment = frontend->AttachmentAllocate();
  offscreen_depth_attachment->Create(
      GPU_FORMAT_DEVICE_DEPTH_OPTIMAL,
      GPU_ATTACHMENT_USAGE_DEPTH_STENCIL_ATTACHMENT, width, height);
  std::vector<GPUAttachment *> offscreen_attachments;
  offscreen_attachments.emplace_back(offscreen_color_attachment);
  offscreen_attachments.emplace_back(offscreen_depth_attachment);

  GPURenderPass *offscreen_render_pass = frontend->RenderPassAllocate();
  offscreen_render_pass->Create(
      std::vector<GPURenderPassAttachmentConfig>{
          GPURenderPassAttachmentConfig{
              GPU_FORMAT_DEVICE_COLOR_OPTIMAL,
              GPU_ATTACHMENT_USAGE_COLOR_ATTACHMENT,
              GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_DONT_CARE,
              GPU_RENDER_PASS_ATTACHMENT_STORE_OPERATION_STORE, false},
          GPURenderPassAttachmentConfig{
              GPU_FORMAT_DEVICE_DEPTH_OPTIMAL,
              GPU_ATTACHMENT_USAGE_DEPTH_STENCIL_ATTACHMENT,
              GPU_RENDER_PASS_ATTACHMENT_LOAD_OPERATION_DONT_CARE,
              GPU_RENDER_PASS_ATTACHMENT_STORE_OPERATION_DONT_CARE, false}},
      glm::vec4(0, 0, width, height), glm::vec4(0, 0, 0, 1), 1.0f, 0.0f,
      GPU_RENDER_PASS_CLEAR_FLAG_COLOR | GPU_RENDER_PASS_CLEAR_FLAG_DEPTH |
          GPU_RENDER_PASS_CLEAR_FLAG_STENCIL);
  GPURenderTarget *offscreen_render_target = frontend->RenderTargetAllocate();
  offscreen_render_target->Create(offscreen_render_pass, offscreen_attachments,
                                  width, height);

  std::vector<GPUShaderStageConfig> stage_configs;
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_VERTEX, "../../assets/shaders/textures.vert.spv"});
  stage_configs.emplace_back(
      GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_FRAGMENT,
                           "../../assets/shaders/textures.frag.spv"});
  if (!shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                      GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                          GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                      offscreen_render_pass, width, height)) {
    FATAL("Failed to create a shader. Aborting...");
    exit(1);
  }

  GPUUniformBuffer *global_uniform = frontend->UniformBufferAllocate();
  GPUUniformBuffer *instance_uniform = frontend->UniformBufferAllocate();

  global_uniform->Create(sizeof(GlobalUBO));
  instance_uniform->Create(sizeof(InstanceUBO), meshes.size());

  GPUDescriptorSet *global_descriptor_set = frontend->DescriptorSetAllocate();
  GPUDescriptorSet *instance_descriptor_set = frontend->DescriptorSetAllocate();
  for (int i = 0; i < meshes.size(); ++i) {
    meshes[i].texture_descriptor_set = frontend->DescriptorSetAllocate();
  }

  std::vector<GPUDescriptorBinding> bindings;

  bindings.emplace_back(GPUDescriptorBinding{
      0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, global_uniform});
  global_descriptor_set->Create(bindings);
  bindings.clear();

  bindings.emplace_back(GPUDescriptorBinding{
      0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, instance_uniform});
  instance_descriptor_set->Create(bindings);
  bindings.clear();

  for (int i = 0; i < meshes.size(); ++i) {
    bindings.emplace_back(GPUDescriptorBinding{
        0, GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE, meshes[i].texture, 0});
    meshes[i].texture_descriptor_set->Create(bindings);
    bindings.clear();
  }

  Camera *camera = new Camera();
  camera->Create(45, width / height, 0.1f, 1000.0f);
  camera->SetViewportSize(width, height);

  GPUShader *post_processing_shader = frontend->ShaderAllocate();
  stage_configs.clear();
  stage_configs.emplace_back(
      GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_VERTEX,
                           "../../assets/shaders/postprocessing.vert.spv"});
  stage_configs.emplace_back(
      GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_FRAGMENT,
                           "../../assets/shaders/postprocessing.frag.spv"});
  post_processing_shader->Create(stage_configs,
                                 GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                                 GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                                     GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                                 window_render_pass, width, height);

  GPUDescriptorSet *post_processing_set = frontend->DescriptorSetAllocate();
  bindings.clear();
  bindings.emplace_back(
      GPUDescriptorBinding{0, GPU_DESCRIPTOR_BINDING_TYPE_ATTACHMENT, 0, 0,
                           offscreen_color_attachment});
  post_processing_set->Create(bindings);

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
      if (Input::WasKeyHeld(SDLK_LSHIFT)) {
        camera->Pan(mouse_delta);
      } else {
        camera->Rotate(mouse_delta);
      }
    }
    if (wheel_movement.y != 0) {
      camera->Zoom(delta_time * wheel_movement.y);
    }

    if (frontend->BeginFrame()) {
      offscreen_render_pass->Begin(offscreen_render_target);

      static float angle = 0.0f;
      angle += 0.003f;

      glm::vec3 camera_position = glm::vec3(0, 0, 0.0f);
      GlobalUBO global_ubo = {};
      global_ubo.view = camera->GetViewMatrix();
      global_ubo.projection = camera->GetProjectionMatrix();
      global_uniform->LoadData(0, global_uniform->GetSize(), &global_ubo);

      shader->Bind();
      vertex_buffer->Bind(0);
      shader->BindUniformBuffer(global_descriptor_set, 0, 0);

      for (int i = 0; i < meshes.size(); ++i) {
        std::vector<InstanceUBO> instance_ubos;
        for (int j = 0; j < meshes.size(); ++j) {
          InstanceUBO instance_ubo = {};
          instance_ubo.model = glm::mat4(1.0f);
          instance_ubo.model =
              glm::translate(instance_ubo.model, meshes[j].position) *
              glm::rotate(instance_ubo.model, angle,
                          glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f)));

          instance_ubos.emplace_back(instance_ubo);
        }

        instance_uniform->LoadData(0, instance_uniform->GetSize(),
                                   instance_ubos.data());
        shader->BindUniformBuffer(instance_descriptor_set,
                                  i * instance_uniform->GetDynamicAlignment(),
                                  1);

        shader->BindSampler(meshes[i].texture_descriptor_set, 2);

        frontend->Draw(vertices.size() / 8);
      }

      offscreen_render_pass->End();

      window_render_pass->Begin(frontend->GetCurrentWindowRenderTarget());

      post_processing_shader->Bind();
      post_processing_shader->BindSampler(post_processing_set, 0);
      frontend->Draw(4);

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

  post_processing_set->Destroy();
  delete post_processing_set;
  post_processing_shader->Destroy();
  delete post_processing_shader;

  offscreen_render_target->Destroy();
  offscreen_depth_attachment->Destroy();
  offscreen_color_attachment->Destroy();
  offscreen_render_pass->Destroy();
  delete offscreen_render_pass;
  for (int i = 0; i < meshes.size(); ++i) {
    meshes[i].texture->Destroy();
    delete meshes[i].texture;
  }
  instance_uniform->Destroy();
  delete instance_uniform;
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