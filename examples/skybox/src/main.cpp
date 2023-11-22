#include <iostream>

#include "../../base/camera.h"
#include "../../base/input.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rf3d/logger.h>
#include <rf3d/renderer/renderer_frontend.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

struct GlobalUBO {
  glm::mat4 view;
  glm::mat4 projection;
};

struct InstanceUBO {
  glm::mat4 model;
};

std::vector<float> GenerateSphereVertices(float radius, int sectorCount,
                                          int stackCount) {
  std::vector<float> vertices;

  const float PI = acos(-1.0f);

  float x, y, z, xy;                           // vertex position
  float nx, ny, nz, lengthInv = 1.0f / radius; // normal
  float s, t;                                  // texCoord

  float sectorStep = 2 * PI / sectorCount;
  float stackStep = PI / stackCount;
  float sectorAngle, stackAngle;

  for (int i = 0; i <= stackCount; ++i) {
    stackAngle = PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
    xy = radius * cosf(stackAngle);      // r * cos(u)
    z = radius * sinf(stackAngle);       // r * sin(u)

    // add (sectorCount+1) vertices per stack
    // the first and last vertices have same position and normal, but different
    // tex coords
    for (int j = 0; j <= sectorCount; ++j) {
      sectorAngle = j * sectorStep; // starting from 0 to 2pi

      // vertex position
      x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
      y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      // normalized vertex normal
      nx = x * lengthInv;
      ny = y * lengthInv;
      nz = z * lengthInv;
      vertices.push_back(nx);
      vertices.push_back(ny);
      vertices.push_back(nz);

      // vertex tex coord between [0, 1]
      s = (float)j / sectorCount;
      t = (float)i / stackCount;
      vertices.push_back(s);
      vertices.push_back(t);
    }
  }

  return vertices;
}

std::vector<unsigned int> GenerateSphereIndices(int sectorCount,
                                                int stackCount) {
  std::vector<unsigned int> indices;

  // indices
  //  k1--k1+1
  //  |  / |
  //  | /  |
  //  k2--k2+1
  unsigned int k1, k2;
  for (int i = 0; i < stackCount; ++i) {
    k1 = i * (sectorCount + 1); // beginning of current stack
    k2 = k1 + sectorCount + 1;  // beginning of next stack

    for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
      // 2 triangles per sector excluding 1st and last stacks
      if (i != 0) {
        indices.push_back(k1);
        indices.push_back(k2);
        indices.push_back(k1 + 1);
      }

      if (i != (stackCount - 1)) {
        indices.push_back(k1 + 1);
        indices.push_back(k2);
        indices.push_back(k2 + 1);
      }
    }
  }

  return indices;
}

void LoadCubemap(GPUTexture *texture, std::array<const char *, 6> paths) {
  int texture_width, texture_height, texture_num_channels;
  unsigned char *data = 0;

  for (int i = 0; i < paths.size(); ++i) {
    unsigned char *texture_data =
        stbi_load(paths[i], &texture_width, &texture_height,
                  &texture_num_channels, STBI_rgb_alpha);
    if (!texture_data) {
      FATAL("Failed to load image!");
      return;
    }

    int image_size = texture_width * texture_height * 4;

    if (!data) {
      data = (unsigned char *)malloc(sizeof(*data) * image_size * 6);
    }

    memcpy(data + image_size * i, texture_data, image_size);

    stbi_image_free(texture_data);
  }

  texture->Create(GPU_FORMAT_RGBA8, GPU_TEXTURE_TYPE_CUBEMAP, texture_width,
                  texture_height);
  texture->WriteData(data, 0);

  free(data);
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
                       width, height, SDL_WINDOW_VULKAN);
  RendererFrontend *frontend = new RendererFrontend();
  if (!frontend->Initialize(window, RendererBackendType::RBT_VULKAN)) {
    exit(1);
  }

  GPURenderPass *window_render_pass = frontend->GetWindowRenderPass();

  GPUUniformBuffer *global_uniform = frontend->UniformBufferAllocate();
  GPUUniformBuffer *instance_uniform = frontend->UniformBufferAllocate();

  global_uniform->Create(sizeof(GlobalUBO));
  instance_uniform->Create(sizeof(InstanceUBO));

  GPUDescriptorSet *global_descriptor_set = frontend->DescriptorSetAllocate();
  GPUDescriptorSet *instance_descriptor_set = frontend->DescriptorSetAllocate();

  std::vector<GPUDescriptorBinding> bindings;

  bindings.emplace_back(GPUDescriptorBinding{
      0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, global_uniform});
  global_descriptor_set->Create(bindings);
  bindings.clear();

  bindings.emplace_back(GPUDescriptorBinding{
      0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, instance_uniform});
  instance_descriptor_set->Create(bindings);
  bindings.clear();

  Camera *camera = new Camera();
  camera->Create(45, width / height, 0.1f, 1000.0f);
  camera->SetViewportSize(width, height);

  GPUTexture *cubemap = frontend->TextureAllocate();
  std::array<const char *, 6> cubemap_paths;
  cubemap_paths[0] = "../../assets/textures/skybox_r.jpg";
  cubemap_paths[1] = "../../assets/textures/skybox_l.jpg";
  cubemap_paths[2] = "../../assets/textures/skybox_u.jpg";
  cubemap_paths[3] = "../../assets/textures/skybox_d.jpg";
  cubemap_paths[4] = "../../assets/textures/skybox_f.jpg";
  cubemap_paths[5] = "../../assets/textures/skybox_b.jpg";
  LoadCubemap(cubemap, cubemap_paths);

  std::vector<GPUShaderStageConfig> stage_configs;
  GPUShader *skybox_shader = frontend->ShaderAllocate();
  stage_configs.clear();
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_VERTEX, "../../assets/shaders/skybox.vert.spv"});
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_FRAGMENT, "../../assets/shaders/skybox.frag.spv"});
  skybox_shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                        0, window_render_pass, width, height);
  GPUDescriptorSet *skybox_texture_set = frontend->DescriptorSetAllocate();
  bindings.clear();
  bindings.emplace_back(GPUDescriptorBinding{
      0, GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE, cubemap, 0, 0});
  skybox_texture_set->Create(bindings);

  std::vector<float> skybox_vertices = {
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

      1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

      -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

  GPUVertexBuffer *skybox_vertex_buffer = frontend->VertexBufferAllocate();
  skybox_vertex_buffer->Create(skybox_vertices.size() *
                               sizeof(skybox_vertices[0]));
  skybox_vertex_buffer->LoadData(
      0, skybox_vertices.size() * sizeof(skybox_vertices[0]),
      skybox_vertices.data());

  std::vector<float> sphere_vertices = GenerateSphereVertices(1, 36, 18);
  GPUVertexBuffer *sphere_vertex_buffer = frontend->VertexBufferAllocate();
  sphere_vertex_buffer->Create(sphere_vertices.size() *
                               sizeof(sphere_vertices[0]));
  sphere_vertex_buffer->LoadData(
      0, sphere_vertices.size() * sizeof(sphere_vertices[0]),
      sphere_vertices.data());

  std::vector<unsigned int> sphere_indices = GenerateSphereIndices(36, 18);
  GPUIndexBuffer *sphere_index_buffer = frontend->IndexBufferAllocate();
  sphere_index_buffer->Create(sphere_indices.size() *
                              sizeof(sphere_indices[0]));
  sphere_index_buffer->LoadData(
      0, sphere_indices.size() * sizeof(sphere_indices[0]),
      sphere_indices.data());

  GPUShader *reflect_shader = frontend->ShaderAllocate();
  stage_configs.clear();
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_VERTEX, "../../assets/shaders/reflect.vert.spv"});
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_FRAGMENT, "../../assets/shaders/reflect.frag.spv"});
  reflect_shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                         GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                             GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                         window_render_pass, width, height);

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
      window_render_pass->Begin(frontend->GetCurrentWindowRenderTarget());

      GlobalUBO global_ubo = {};
      global_ubo.view = camera->GetViewMatrix();
      global_ubo.projection = camera->GetProjectionMatrix();
      global_uniform->LoadData(0, global_uniform->GetSize(), &global_ubo);

      skybox_shader->Bind();
      skybox_vertex_buffer->Bind(0);
      skybox_shader->BindUniformBuffer(global_descriptor_set, 0, 0);
      skybox_shader->BindSampler(skybox_texture_set, 1);
      frontend->Draw(skybox_vertices.size() / 3);

      InstanceUBO instance_ubo = {};
      instance_ubo.model = glm::mat4(1.0f);
      instance_ubo.model = glm::translate(instance_ubo.model, glm::vec3(0.0f));
      instance_uniform->LoadData(0, sizeof(InstanceUBO), &instance_ubo);

      reflect_shader->Bind();
      sphere_vertex_buffer->Bind(0);
      sphere_index_buffer->Bind(0);
      reflect_shader->BindUniformBuffer(global_descriptor_set, 0, 0);
      reflect_shader->BindUniformBuffer(instance_descriptor_set, 0, 1);
      reflect_shader->BindSampler(skybox_texture_set, 2);
      frontend->DrawIndexed(sphere_indices.size());

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

  sphere_vertex_buffer->Destroy();
  delete sphere_vertex_buffer;

  sphere_index_buffer->Destroy();
  delete sphere_index_buffer;

  reflect_shader->Destroy();
  delete reflect_shader;

  cubemap->Destroy();
  delete cubemap;

  skybox_vertex_buffer->Destroy();
  delete skybox_vertex_buffer;

  skybox_texture_set->Destroy();
  delete skybox_texture_set;

  skybox_shader->Destroy();
  delete skybox_shader;

  instance_uniform->Destroy();
  delete instance_uniform;
  global_uniform->Destroy();
  delete global_uniform;

  frontend->Shutdown();
  delete frontend;

  SDL_DestroyWindow(window);

  return 0;
}