#include <stdio.h>

#include "camera.h"
#include "input.h"
#include "logger.h"
#include "renderer/renderer_frontend.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <SDL2/SDL.h>
#include <array>
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
  GPUDescriptorSet *texture_descriptor_set;
  GPUTexture *texture;
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

  LoadTexture(meshes[0].texture, "assets/textures/metal.png");
  meshes[0].position = glm::vec3(0, 0, 5.0f);
  meshes[0].push_constants =
      PushConsts{0.1f, 1.0f, 0.672411f, 0.637331f, 0.585456f};
  LoadTexture(meshes[1].texture, "assets/textures/wood.png");
  meshes[1].position = glm::vec3(2, 0, 5.0f);
  meshes[1].push_constants = PushConsts{0.8f, 0.2f, 1, 0, 0};
  LoadTexture(meshes[2].texture, "assets/textures/brickwall.jpg");
  meshes[2].position = glm::vec3(-2, 0, 5.0f);
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
      GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/object.vert.spv"});
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/object.frag.spv"});
  if (!shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                      GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                          GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                      offscreen_render_pass, width, height)) {
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
  for (int i = 0; i < meshes.size(); ++i) {
    meshes[i].texture_descriptor_set = frontend->DescriptorSetAllocate();
  }

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

  for (int i = 0; i < meshes.size(); ++i) {
    bindings.emplace_back(GPUDescriptorBinding{
        0, GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE, meshes[i].texture, 0});
    meshes[i].texture_descriptor_set->Create(bindings);
    bindings.clear();
  }

  Camera *camera = new Camera();
  camera->Create(45, width / height, 0.1f, 1000.0f);
  camera->SetViewportSize(width, height);

  const float far = 1000.0f;
  glm::vec3 camera_position = camera->GetPosition();
  std::vector<float> grid_vertices;
  for (float x = camera_position.x - far; x < camera_position.x + far;
       x += 0.5f) {
    glm::vec3 start = glm::vec3((int)x, 0, (int)(camera_position.z - far));
    glm::vec3 end = glm::vec3((int)x, 0, (int)(camera_position.z + far));
    glm::vec3 color = glm::vec3(0.4, 0.4, 0.4);
    if ((int)x == 0) {
      color = glm::vec3(1, 0.4, 0.4);
    }

    grid_vertices.push_back(start.x);
    grid_vertices.push_back(start.y);
    grid_vertices.push_back(start.z);

    grid_vertices.push_back(color.x);
    grid_vertices.push_back(color.y);
    grid_vertices.push_back(color.z);

    grid_vertices.push_back(end.x);
    grid_vertices.push_back(end.y);
    grid_vertices.push_back(end.z);

    grid_vertices.push_back(color.x);
    grid_vertices.push_back(color.y);
    grid_vertices.push_back(color.z);
  }

  for (float z = camera_position.z - far; z < camera_position.z + far;
       z += 0.5f) {
    glm::vec3 start = glm::vec3((int)(camera_position.x - far), 0, (int)z);
    glm::vec3 end = glm::vec3((int)(camera_position.x + far), 0, (int)z);
    glm::vec3 color = glm::vec3(0.4, 0.4, 0.4);
    if ((int)z == 0) {
      color = glm::vec3(0.55, 0.8, 0.9);
    }

    grid_vertices.push_back(start.x);
    grid_vertices.push_back(start.y);
    grid_vertices.push_back(start.z);

    grid_vertices.push_back(color.x);
    grid_vertices.push_back(color.y);
    grid_vertices.push_back(color.z);

    grid_vertices.push_back(end.x);
    grid_vertices.push_back(end.y);
    grid_vertices.push_back(end.z);

    grid_vertices.push_back(color.x);
    grid_vertices.push_back(color.y);
    grid_vertices.push_back(color.z);
  }

  GPUShader *grid_shader = frontend->ShaderAllocate();
  stage_configs.clear();
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/grid.vert.spv"});
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/grid.frag.spv"});
  grid_shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_LINE_LIST,
                      GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                          GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                      offscreen_render_pass, width, height);

  GPUVertexBuffer *grid_vertex_buffer = frontend->VertexBufferAllocate();
  grid_vertex_buffer->Create(grid_vertices.size() * sizeof(grid_vertices[0]));
  grid_vertex_buffer->LoadData(
      0, grid_vertices.size() * sizeof(grid_vertices[0]), grid_vertices.data());

  GPUShader *post_processing_shader = frontend->ShaderAllocate();
  stage_configs.clear();
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/postprocessing.vert.spv"});
  stage_configs.emplace_back(
      GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_FRAGMENT,
                           "assets/shaders/postprocessing.frag.spv"});
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

  GPUTexture *cubemap = frontend->TextureAllocate();
  std::array<const char *, 6> cubemap_paths;
  cubemap_paths[0] = "assets/textures/skybox_r.jpg";
  cubemap_paths[1] = "assets/textures/skybox_l.jpg";
  cubemap_paths[2] = "assets/textures/skybox_u.jpg";
  cubemap_paths[3] = "assets/textures/skybox_d.jpg";
  cubemap_paths[4] = "assets/textures/skybox_f.jpg";
  cubemap_paths[5] = "assets/textures/skybox_b.jpg";
  LoadCubemap(cubemap, cubemap_paths);

  GPUShader *skybox_shader = frontend->ShaderAllocate();
  stage_configs.clear();
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/skybox.vert.spv"});
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/skybox.frag.spv"});
  skybox_shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                        0, offscreen_render_pass, width, height);
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
      GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/reflect.vert.spv"});
  stage_configs.emplace_back(GPUShaderStageConfig{
      GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/reflect.frag.spv"});
  reflect_shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                         GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                             GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                         offscreen_render_pass, width, height);

  GPUUniformBuffer *reflect_ubo = frontend->UniformBufferAllocate();
  reflect_ubo->Create(sizeof(InstanceUBO));

  GPUDescriptorSet *reflect_instance_set = frontend->DescriptorSetAllocate();
  bindings.clear();
  bindings.emplace_back(GPUDescriptorBinding{
      0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, reflect_ubo, 0});
  reflect_instance_set->Create(bindings);

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

      skybox_shader->Bind();
      skybox_vertex_buffer->Bind(0);
      skybox_shader->BindUniformBuffer(global_descriptor_set, 0, 0);
      skybox_shader->BindSampler(skybox_texture_set, 1);
      frontend->Draw(skybox_vertices.size() / 3);

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
              glm::translate(instance_ubo.model, meshes[j].position) *
              glm::rotate(instance_ubo.model, angle,
                          glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f)));

          instance_ubos.emplace_back(instance_ubo);
        }

        instance_uniform->LoadData(0, instance_uniform->GetSize(),
                                   instance_ubos.data());
        shader->BindUniformBuffer(instance_descriptor_set,
                                  i * instance_uniform->GetDynamicAlignment(),
                                  2);

        shader->BindSampler(meshes[i].texture_descriptor_set, 3);

        shader->PushConstant(&meshes[i].push_constants, sizeof(PushConsts), 0,
                             GPU_SHADER_STAGE_TYPE_FRAGMENT);

        frontend->Draw(vertices.size() / 8);
      }

      grid_shader->Bind();
      grid_vertex_buffer->Bind(0);
      grid_shader->BindUniformBuffer(global_descriptor_set, 0, 0);
      frontend->Draw(grid_vertices.size() / 6);

      InstanceUBO instance_ubo = {};
      instance_ubo.model = glm::mat4(1.0f);
      instance_ubo.model = glm::translate(instance_ubo.model, glm::vec3(0.0f));
      reflect_ubo->LoadData(0, sizeof(InstanceUBO), &instance_ubo);

      reflect_shader->Bind();
      sphere_vertex_buffer->Bind(0);
      sphere_index_buffer->Bind(0);
      reflect_shader->BindUniformBuffer(global_descriptor_set, 0, 0);
      reflect_shader->BindUniformBuffer(world_descriptor_set, 0, 1);
      reflect_shader->BindUniformBuffer(reflect_instance_set, 0, 2);
      reflect_shader->BindSampler(skybox_texture_set, 3);
      frontend->DrawIndexed(sphere_indices.size());

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

  sphere_vertex_buffer->Destroy();
  delete sphere_vertex_buffer;

  sphere_index_buffer->Destroy();
  delete sphere_index_buffer;

  reflect_ubo->Destroy();
  delete reflect_ubo;

  reflect_instance_set->Destroy();
  delete reflect_instance_set;

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

  post_processing_set->Destroy();
  delete post_processing_set;
  post_processing_shader->Destroy();
  delete post_processing_shader;

  grid_vertex_buffer->Destroy();
  delete grid_vertex_buffer;
  grid_shader->Destroy();
  delete grid_shader;

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