#include <stdio.h>

#include "logger.h"
#include "renderer/renderer_frontend.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct PushConsts {
  float roughness;
  float metallic;
  float r;
  float g;
  float b;
};

struct MeshParams {
  // GPUTexture *texture;
  glm::vec3 position;
  PushConsts push_constants;
};

struct GlobalUBO {
  glm::mat4 view;
  glm::mat4 projection;
};

struct InstanceUBO {
  glm::mat4 model;
};

void load_texture(GPUTexture *texture, const char *path) {
  int texture_width, texture_height, texture_num_channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path, &texture_width, &texture_height,
                                  &texture_num_channels, 0);
  if (!data) {
    FATAL("Failed to load image!");
  }

  texture->Create(GPU_FORMAT_RGB8, GPU_TEXTURE_TYPE_2D, texture_width,
                  texture_height);
  texture->WriteData(data, 0);

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
    // meshes[i].texture = frontend->TextureAllocate();
  }

  // loadTexture(meshes[0].texture, "assets/textures/metal.png");
  meshes[0].position = glm::vec3(0, 0, 0.0f);
  meshes[0].push_constants =
      PushConsts{0.1f, 1.0f, 0.672411f, 0.637331f, 0.585456f};
  // loadTexture(meshes[1].texture, "assets/textures/brickwall.jpg");
  meshes[1].position = glm::vec3(2, 0, 0.0f);
  meshes[1].push_constants = PushConsts{0.8f, 0.2f, 1, 0, 0};
  // loadTexture(meshes[2].texture, "assets/textures/wood.png");
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

  GPUShaderConfig shader_config = {};
  shader_config.stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/object_shader.vert.spv"});
  shader_config.stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/object_shader.frag.spv"});
  if (!shader->Create(&shader_config, window_render_pass, width, height)) {
    FATAL("Failed to create a shader. Aborting...");
    exit(1);
  }

  shader->PrepareShaderBuffer({0, 0}, sizeof(GlobalUBO));
  shader->PrepareShaderBuffer({1, 0}, sizeof(InstanceUBO), meshes.size());

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT: {
        running = false;
      } break;
      case SDL_WINDOWEVENT: {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          int width, height;
          SDL_GetWindowSize(window, &width, &height);

          frontend->Resize((uint32_t)width, (uint32_t)height);
        }
      } break;
      }
    }

    if (frontend->BeginFrame()) {
      window_render_pass->Begin(frontend->GetCurrentWindowRenderTarget());

      GPUShaderPushConstant push_constant;
      push_constant.size = sizeof(PushConsts);
      push_constant.offset = 0;
      push_constant.stage_flags = GPU_SHADER_STAGE_TYPE_FRAGMENT;

      static float angle = 0.0f;
      angle += 0.003f;

      shader->Bind();
      vertex_buffer->Bind(0);

      glm::vec3 camera_position = glm::vec3(0, 0, -5.0f);
      GlobalUBO global_ubo = {};
      global_ubo.view = glm::translate(glm::mat4(1.0f), camera_position);
      global_ubo.view = glm::inverse(global_ubo.view);
      global_ubo.projection = glm::perspective(
          glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
      shader->GetShaderBuffer({0, 0})->LoadData(
          0, shader->GetShaderBuffer({0, 0})->GetSize(), &global_ubo);
      shader->BindShaderBuffer({0, 0});

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

        // UBOLights ubo_lights = {};
        // const float p = 5.0f;
        // ubo_lights.lights[0] = glm::vec4(-p * 0.8f, -p * 0.8f, p *
        // 0.8f, 1.0f); ubo_lights.lights[1] = glm::vec4(-p * 2, p * 2, p *
        // 2, 1.0f); ubo_lights.lights[2] = glm::vec4(p * 0.2f, -p * 0.2f, p *
        // 0.2f, 1.0f); ubo_lights.lights[3] = glm::vec4(p, p, p, 1.0f);

        shader->GetShaderBuffer({1, 0})->LoadData(
            0, shader->GetShaderBuffer({1, 0})->GetSize(),
            instance_ubos.data());
        shader->BindShaderBuffer({1, 0}, i);
        // meshes[i].lights_buffer->GetBuffer()->LoadData(
        //     0, meshes[i].lights_buffer->GetBuffer()->GetSize(), &ubo_lights);
        // meshes[i].lights_buffer->Bind(shader);

        push_constant.value = &meshes[i].push_constants;
        shader->PushConstant(&push_constant);
        // shader->SetTexture(0, meshes[i].texture);

        frontend->Draw(vertices.size());
      }

      window_render_pass->End();

      frontend->EndFrame();
    }
  }

  for (int i = 0; i < meshes.size(); ++i) {
    // meshes[i].texture->Destroy();
    // delete meshes[i].texture;
  }
  shader->Destroy();
  delete shader;
  vertex_buffer->Destroy();
  delete vertex_buffer;

  frontend->Shutdown();
  delete frontend;

  SDL_DestroyWindow(window);

  return 0;
}