#include <iostream>

#include "../base/example.h"
#include "../base/utils.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rf3d/framework/logger.h>
#include <rf3d/framework/renderer/renderer_frontend.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

class SkyboxExample : public Example {
public:
  SkyboxExample(const char *example_name, int window_width, int window_height)
      : Example(example_name, window_width, window_height) {

    global_uniform = frontend->UniformBufferAllocate();
    global_uniform->Create(sizeof(GlobalUBO));
    global_uniform->SetDebugName("Global uniform buffer");

    instance_uniform = frontend->UniformBufferAllocate();
    instance_uniform->Create(sizeof(InstanceUBO));
    instance_uniform->SetDebugName("Instance uniform buffer");

    std::vector<GPUDescriptorBinding> bindings;

    global_descriptor_set = frontend->DescriptorSetAllocate();
    bindings.emplace_back(GPUDescriptorBinding{
        0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, global_uniform});
    global_descriptor_set->Create(bindings);
    global_descriptor_set->SetDebugName("Global descriptor set");
    bindings.clear();

    instance_descriptor_set = frontend->DescriptorSetAllocate();
    bindings.emplace_back(GPUDescriptorBinding{
        0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, instance_uniform});
    instance_descriptor_set->Create(bindings);
    instance_descriptor_set->SetDebugName("Instance descriptor set");
    bindings.clear();

    cubemap = frontend->TextureAllocate();
    std::array<const char *, 6> cubemap_paths;
    cubemap_paths[0] = "assets/textures/skybox_r.jpg";
    cubemap_paths[1] = "assets/textures/skybox_l.jpg";
    cubemap_paths[2] = "assets/textures/skybox_u.jpg";
    cubemap_paths[3] = "assets/textures/skybox_d.jpg";
    cubemap_paths[4] = "assets/textures/skybox_f.jpg";
    cubemap_paths[5] = "assets/textures/skybox_b.jpg";
    Utils::LoadCubemap(cubemap, cubemap_paths);
    cubemap->SetDebugName("Cubemap texture");

    std::vector<GPUShaderStageConfig> stage_configs;
    skybox_shader = frontend->ShaderAllocate();
    stage_configs.clear();
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/skybox.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/skybox.frag.spv"});
    skybox_shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                          0, frontend->GetWindowRenderPass(), width, height);
    skybox_shader->SetDebugName("Skybox shader");

    skybox_texture_set = frontend->DescriptorSetAllocate();
    bindings.clear();
    bindings.emplace_back(GPUDescriptorBinding{
        0, GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE, cubemap, 0, 0});
    skybox_texture_set->Create(bindings);
    skybox_texture_set->SetDebugName("Skybox texture descriptor set");

    skybox_vertices = Utils::GetCubeVerticesPositionsOnly();
    skybox_vertex_buffer = frontend->VertexBufferAllocate();
    skybox_vertex_buffer->Create(skybox_vertices.size() *
                                 sizeof(skybox_vertices[0]));
    skybox_vertex_buffer->LoadData(
        0, skybox_vertices.size() * sizeof(skybox_vertices[0]),
        skybox_vertices.data());
    skybox_vertex_buffer->SetDebugName("Skybox vertex buffer");

    sphere_vertices = Utils::GenerateSphereVertices(1, 36, 18);
    sphere_indices = Utils::GenerateSphereIndices(36, 18);

    sphere_vertex_buffer = frontend->VertexBufferAllocate();
    sphere_vertex_buffer->Create(sphere_vertices.size() *
                                 sizeof(sphere_vertices[0]));
    sphere_vertex_buffer->LoadData(
        0, sphere_vertices.size() * sizeof(sphere_vertices[0]),
        sphere_vertices.data());
    sphere_vertex_buffer->SetDebugName("Sphere vertex buffer");

    sphere_index_buffer = frontend->IndexBufferAllocate();
    sphere_index_buffer->Create(sphere_indices.size() *
                                sizeof(sphere_indices[0]));
    sphere_index_buffer->LoadData(
        0, sphere_indices.size() * sizeof(sphere_indices[0]),
        sphere_indices.data());
    sphere_index_buffer->SetDebugName("Sphere index buffer");

    reflect_shader = frontend->ShaderAllocate();
    stage_configs.clear();
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/reflect.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/reflect.frag.spv"});
    reflect_shader->Create(stage_configs,
                           GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                           GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                               GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                           frontend->GetWindowRenderPass(), width, height);
    reflect_shader->SetDebugName("Reflect shader");
  }

  virtual ~SkyboxExample() {
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

    global_descriptor_set->Destroy();
    delete global_descriptor_set;
    instance_descriptor_set->Destroy();
    delete instance_descriptor_set;

    instance_uniform->Destroy();
    delete instance_uniform;
    global_uniform->Destroy();
    delete global_uniform;
  }

  void EventLoop() override {
    while (running) {
      UpdateStart();

      if (frontend->BeginFrame()) {
        frontend->GetWindowRenderPass()->Begin(
            frontend->GetCurrentWindowRenderTarget());
        frontend->BeginDebugRegion("Main pass", glm::vec4(0.0, 1.0, 0.0, 1.0));

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
        instance_ubo.model =
            glm::translate(instance_ubo.model, glm::vec3(0.0f));
        instance_uniform->LoadData(0, sizeof(InstanceUBO), &instance_ubo);

        reflect_shader->Bind();
        sphere_vertex_buffer->Bind(0);
        sphere_index_buffer->Bind(0);
        reflect_shader->BindUniformBuffer(global_descriptor_set, 0, 0);
        reflect_shader->BindUniformBuffer(instance_descriptor_set, 0, 1);
        reflect_shader->BindSampler(skybox_texture_set, 2);
        frontend->DrawIndexed(sphere_indices.size());

        frontend->EndDebugRegion();
        frontend->GetWindowRenderPass()->End();

        frontend->EndFrame();
      }

      UpdateEnd();
    }
  }

private:
  struct GlobalUBO {
    glm::mat4 view;
    glm::mat4 projection;
  };
  struct InstanceUBO {
    glm::mat4 model;
  };

  GPUUniformBuffer *global_uniform;
  GPUUniformBuffer *instance_uniform;
  GPUDescriptorSet *global_descriptor_set;
  GPUDescriptorSet *instance_descriptor_set;

  GPUTexture *cubemap;

  GPUShader *skybox_shader;
  GPUDescriptorSet *skybox_texture_set;

  std::vector<float> skybox_vertices;
  GPUVertexBuffer *skybox_vertex_buffer;

  std::vector<float> sphere_vertices;
  std::vector<unsigned int> sphere_indices;
  GPUVertexBuffer *sphere_vertex_buffer;
  GPUIndexBuffer *sphere_index_buffer;

  GPUShader *reflect_shader;
};

int main(int argc, char **argv) {
  Example *example = new SkyboxExample("Skybox", 800, 600);
  example->EventLoop();
  delete example;
}