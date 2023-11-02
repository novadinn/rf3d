#include <stdio.h>

#include "logger.h"
#include "renderer/renderer_frontend.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define WITH_VULKAN_BACKEND 0

struct PushConsts {
  float roughness;
  float metallic;
  float r;
  float g;
  float b;
};

struct MeshParams {
  GPUShaderBuffer *shader_buffer;
  GPUShaderBuffer *lights_buffer;
  GPUTexture *texture;
  glm::vec3 position;
  PushConsts push_constants;
};

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  glm::mat4 _pad0;
};

struct UBOLights {
  glm::vec4 lights[4];
};

void loadTexture(GPUTexture *texture, const char *path) {
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
  GPURenderPass *window_render_pass = frontend->GetWindowRenderPass();

  std::vector<MeshParams> meshes(3);
  for (int i = 0; i < meshes.size(); ++i) {
    meshes[i].shader_buffer = frontend->ShaderBufferAllocate();
    meshes[i].lights_buffer = frontend->ShaderBufferAllocate();
    meshes[i].texture = frontend->TextureAllocate();
  }

  loadTexture(meshes[0].texture, "assets/textures/metal.png");
  meshes[0].position = glm::vec3(0, 0, -5.0f);
  meshes[0].push_constants =
      PushConsts{0.1f, 1.0f, 0.672411f, 0.637331f, 0.585456f};
  loadTexture(meshes[1].texture, "assets/textures/brickwall.jpg");
  meshes[1].position = glm::vec3(2, 0, -5.0f);
  meshes[1].push_constants = PushConsts{0.8f, 0.2f, 1, 0, 0};
  loadTexture(meshes[2].texture, "assets/textures/wood.png");
  meshes[2].position = glm::vec3(-2, 0, -5.0f);
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

  vertex_buffer->Create(GPU_BUFFER_TYPE_VERTEX,
                        vertices.size() * sizeof(vertices[0]));
  vertex_buffer->LoadData(0, vertices.size() * sizeof(vertices[0]),
                          vertices.data());

  std::vector<GPUFormat> attributes;
  attributes.emplace_back(GPUFormat{GPU_FORMAT_RGB32F});
  attributes.emplace_back(GPUFormat{GPU_FORMAT_RGB32F});
  attributes.emplace_back(GPUFormat{GPU_FORMAT_RG32F});
  attribute_array->Create(vertex_buffer, 0, attributes);

  for (int i = 0; i < meshes.size(); ++i) {
    meshes[i].shader_buffer->Create(
        "uniform_buffer_object", GPU_SHADER_BUFFER_TYPE_UNIFORM_BUFFER,
        GPU_SHADER_STAGE_TYPE_VERTEX, sizeof(glm::mat4) * 4, 0);
    meshes[i].lights_buffer->Create(
        "ubo_lights", GPU_SHADER_BUFFER_TYPE_UNIFORM_BUFFER,
        GPU_SHADER_STAGE_TYPE_FRAGMENT, sizeof(glm::vec3) * 4, 0);
  }

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
  shader_config.descriptors.emplace_back(meshes[0].shader_buffer);
  shader_config.descriptors.emplace_back(meshes[0].lights_buffer);
  shader_config.attribute_configs.emplace_back(
      GPUShaderAttributeConfig{GPU_FORMAT_RGB32F});
  shader_config.attribute_configs.emplace_back(
      GPUShaderAttributeConfig{GPU_FORMAT_RGB32F});
  shader_config.attribute_configs.emplace_back(
      GPUShaderAttributeConfig{GPU_FORMAT_RG32F});
  shader_config.push_constant_configs.emplace_back(GPUShaderPushConstantConfig{
      GPU_SHADER_STAGE_TYPE_FRAGMENT, 0, sizeof(float) * 5});
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

      GPUShaderPushConstant push_constant;
      push_constant.name = "material";
      push_constant.values.emplace_back(GPUShaderPushConstantValue{
          "roughness", GPU_SHADER_GLSL_VALUE_TYPE_FLOAT});
      push_constant.values.emplace_back(GPUShaderPushConstantValue{
          "metallic", GPU_SHADER_GLSL_VALUE_TYPE_FLOAT});
      push_constant.values.emplace_back(
          GPUShaderPushConstantValue{"r", GPU_SHADER_GLSL_VALUE_TYPE_FLOAT});
      push_constant.values.emplace_back(
          GPUShaderPushConstantValue{"g", GPU_SHADER_GLSL_VALUE_TYPE_FLOAT});
      push_constant.values.emplace_back(
          GPUShaderPushConstantValue{"b", GPU_SHADER_GLSL_VALUE_TYPE_FLOAT});
      push_constant.size = sizeof(PushConsts);
      push_constant.offset = 0;
      push_constant.stage_flags = GPU_SHADER_STAGE_TYPE_FRAGMENT;

      static float angle = 0.0f;
      angle += 0.003f;
      for (int i = 0; i < meshes.size(); ++i) {
        UniformBufferObject ubo = {};
        ubo.model = glm::mat4(1.0f);
        ubo.model = glm::rotate(ubo.model, angle,
                                glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f)));
        ubo.view = glm::translate(glm::mat4(1.0f), meshes[i].position);
        ubo.view = glm::inverse(ubo.view);
        ubo.projection = glm::perspective(
            glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);

        UBOLights ubo_lights = {};
        const float p = 5.0f;
        ubo_lights.lights[0] = glm::vec4(-p * 0.8f, -p * 0.8f, p * 0.8f, 1.0f);
        ubo_lights.lights[1] = glm::vec4(-p * 2, p * 2, p * 2, 1.0f);
        ubo_lights.lights[2] = glm::vec4(p * 0.2f, -p * 0.2f, p * 0.2f, 1.0f);
        ubo_lights.lights[3] = glm::vec4(p, p, p, 1.0f);

        shader->Bind();
        meshes[i].shader_buffer->GetBuffer()->LoadData(
            0, meshes[i].shader_buffer->GetBuffer()->GetSize(), &ubo);
        meshes[i].shader_buffer->Bind(shader);
        meshes[i].lights_buffer->GetBuffer()->LoadData(
            0, meshes[i].lights_buffer->GetBuffer()->GetSize(), &ubo_lights);
        meshes[i].lights_buffer->Bind(shader);

        push_constant.value = &meshes[i].push_constants;
        shader->PushConstant(&push_constant);
        shader->SetTexture(0, meshes[i].texture);

        attribute_array->Bind();

        frontend->Draw(vertices.size());
      }

      window_render_pass->End();

      frontend->EndFrame();
    }
  }

  for (int i = 0; i < meshes.size(); ++i) {
    meshes[i].texture->Destroy();
    delete meshes[i].texture;
  }
  shader->Destroy();
  delete shader;
  for (int i = 0; i < meshes.size(); ++i) {
    meshes[i].shader_buffer->Destroy();
    delete meshes[i].shader_buffer;
    meshes[i].lights_buffer->Destroy();
    delete meshes[i].lights_buffer;
  }
  attribute_array->Destroy();
  delete attribute_array;
  vertex_buffer->Destroy();
  delete vertex_buffer;

  frontend->Shutdown();
  delete frontend;

  SDL_DestroyWindow(window);

  return 0;
}