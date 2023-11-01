#include <stdio.h>

#include "logger.h"
#include "renderer/renderer_frontend.h"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define WITH_VULKAN_BACKEND 1

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    FATAL("Couldn't initialize SLD");
    exit(1);
  }

  uint32_t width = 800;
  uint32_t height = 600;

#if WITH_VULKAN_BACKEND == 1
  SDL_Window *window =
      SDL_CreateWindow("Shade a Sphere", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_VULKAN);
  RendererFrontend *frontend = new RendererFrontend();
  if (!frontend->Initialize(window, RendererBackendType::RBT_VULKAN)) {
    exit(1);
  }
#else
  SDL_Window *window =
      SDL_CreateWindow("Shade a Sphere", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
  RendererFrontend *frontend = new RendererFrontend();
  if (!frontend->Initialize(window, RendererBackendType::RBT_OPENGL)) {
    exit(1);
  }
#endif

  GPUBuffer *vertex_buffer = frontend->BufferAllocate();
  GPUAttributeArray *attribute_array = frontend->AttributeArrayAllocate();
  GPUShader *shader = frontend->ShaderAllocate();
  GPUShaderBuffer *uniform_buffer = frontend->ShaderBufferAllocate();
  GPURenderPass *window_render_pass = frontend->GetWindowRenderPass();

  std::vector<float> vertices = {
      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
      1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
      1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
      1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  -1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
      1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f};

  vertex_buffer->Create(GPU_BUFFER_TYPE_VERTEX,
                        vertices.size() * sizeof(vertices[0]));
  vertex_buffer->LoadData(0, vertices.size() * sizeof(vertices[0]),
                          vertices.data());

  std::vector<GPUFormat> attributes;
  attributes.emplace_back(GPUFormat{GPU_FORMAT_RGB32F});
  attribute_array->Create(vertex_buffer, 0, attributes);

  uniform_buffer->Create(
      "uniform_buffer_object", GPU_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      GPU_SHADER_STAGE_TYPE_VERTEX, sizeof(glm::mat4) * 4, 0);

  GPUShaderConfig shader_config = {};
#if WITH_VULKAN_BACKEND == 1
  shader_config.stage_configs.emplace_back(
      GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_VERTEX,
                           "assets/shaders/vulkan/object_shader.vert.spv"});
  shader_config.stage_configs.emplace_back(
      GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_FRAGMENT,
                           "assets/shaders/vulkan/object_shader.frag.spv"});
#else
  shader_config.stage_configs.emplace_back(
      GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_VERTEX,
                           "assets/shaders/opengl/object_shader.vert"});
  shader_config.stage_configs.emplace_back(
      GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_FRAGMENT,
                           "assets/shaders/opengl/object_shader.frag"});
#endif
  shader_config.descriptors.emplace_back(uniform_buffer);
  shader_config.attribute_configs.emplace_back(
      GPUShaderAttributeConfig{GPU_FORMAT_RGB32F});
  if (!shader->Create(&shader_config, window_render_pass, width, height)) {
    FATAL("Failed to create a shader. Aborting...");
    exit(1);
  }

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

      struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 _pad0;
      };
      UniformBufferObject ubo = {};
      ubo.model = glm::mat4(1.0f);
      static float angle = 0.0f;
      angle += 0.003f;
      ubo.model = glm::rotate(ubo.model, angle,
                              glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f)));
      ubo.view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -10.0f));
      ubo.view = glm::inverse(ubo.view);
      ubo.projection = glm::perspective(
          glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
      shader->Bind();
      uniform_buffer->GetBuffer()->LoadData(
          0, uniform_buffer->GetBuffer()->GetSize(), &ubo);
      uniform_buffer->Bind(shader);

      attribute_array->Bind();

      frontend->Draw(vertices.size());

      window_render_pass->End();

      frontend->EndFrame();
    }
  }

  shader->Destroy();
  delete shader;
  uniform_buffer->Destroy();
  delete uniform_buffer;
  attribute_array->Destroy();
  delete attribute_array;
  vertex_buffer->Destroy();
  delete vertex_buffer;

  frontend->Shutdown();
  delete frontend;

  SDL_DestroyWindow(window);

  return 0;
}