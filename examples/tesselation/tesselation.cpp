#include <iostream>

#include "../base/example.h"
#include "../base/mesh.h"
#include "../base/utils.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rf3d/framework/logger.h>
#include <rf3d/framework/renderer/renderer_frontend.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

class TessellationExample : public Example {
public:
  TessellationExample(const char *example_name, int window_width,
                      int window_height)
      : Example(example_name, window_width, window_height) {

    vertices = Utils::GenerateTerrainVertices(20, width, height);

    texture = frontend->TextureAllocate();
    Utils::LoadTexture(texture, "assets/textures/heightmap.png");
    texture->SetDebugName("Height map");

    vertex_buffer = frontend->VertexBufferAllocate();
    vertex_buffer->Create(vertices.size() * sizeof(vertices[0]));
    vertex_buffer->LoadData(0, vertices.size() * sizeof(vertices[0]),
                            vertices.data());
    vertex_buffer->SetDebugName("Cube vertex buffer");

    shader = frontend->ShaderAllocate();
    std::vector<GPUShaderStageConfig> stage_configs;
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/tessellation.vert.spv"});
    stage_configs.emplace_back(
        GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_TESSELLATION_CONTROL,
                             "assets/shaders/tessellation.tesc.spv"});
    stage_configs.emplace_back(
        GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_TESSELLATION_EVALUATION,
                             "assets/shaders/tessellation.tese.spv"});
    stage_configs.emplace_back(
        GPUShaderStageConfig{GPU_SHADER_STAGE_TYPE_FRAGMENT,
                             "assets/shaders/tessellation.frag.spv"});

    shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_PATCH_LIST,
                   GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                       GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                   frontend->GetWindowRenderPass(), width, height);
    shader->SetDebugName("Textures shader");

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

    texture_descriptor_set = frontend->DescriptorSetAllocate();
    bindings.emplace_back(GPUDescriptorBinding{
        0, GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE, texture, 0});
    texture_descriptor_set->Create(bindings);
    texture_descriptor_set->SetDebugName("Texture descriptor set");
    bindings.clear();
  }

  virtual ~TessellationExample() {
    instance_uniform->Destroy();
    delete instance_uniform;
    global_uniform->Destroy();
    delete global_uniform;
    global_descriptor_set->Destroy();
    delete global_descriptor_set;
    instance_descriptor_set->Destroy();
    delete instance_descriptor_set;
    texture_descriptor_set->Destroy();
    delete texture_descriptor_set;

    shader->Destroy();
    delete shader;
    vertex_buffer->Destroy();
    delete vertex_buffer;
    texture->Destroy();
    delete texture;
  }

  void EventLoop() override {
    while (running) {
      UpdateStart();

      if (frontend->BeginFrame()) {
        frontend->GetWindowRenderPass()->Begin(
            frontend->GetCurrentWindowRenderTarget());
        frontend->BeginDebugRegion("Main pass", glm::vec4(0.0, 1.0, 0.0, 1.0));

        glm::vec3 camera_position = glm::vec3(0, 0, 0.0f);
        GlobalUBO global_ubo = {};
        global_ubo.view = camera->GetViewMatrix();
        global_ubo.projection = camera->GetProjectionMatrix();
        global_uniform->LoadData(0, global_uniform->GetSize(), &global_ubo);

        InstanceUBO instance_ubo = {};
        instance_ubo.model = glm::mat4(1.0f);
        instance_ubo.model = glm::translate(instance_ubo.model, glm::vec3(0.0));
        instance_uniform->LoadData(0, instance_uniform->GetSize(),
                                   &instance_ubo);

        shader->Bind();
        vertex_buffer->Bind(0);
        shader->BindUniformBuffer(global_descriptor_set, 0, 0);
        shader->BindUniformBuffer(instance_descriptor_set, 0, 1);
        shader->BindSampler(texture_descriptor_set, 2);

        frontend->Draw(vertices.size() / 5);

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

  std::vector<float> vertices;
  GPUVertexBuffer *vertex_buffer;
  GPUTexture *texture;

  GPUShader *shader;

  GPUUniformBuffer *global_uniform;
  GPUUniformBuffer *instance_uniform;
  GPUDescriptorSet *global_descriptor_set;
  GPUDescriptorSet *instance_descriptor_set;
  GPUDescriptorSet *texture_descriptor_set;
};

int main(int argc, char **argv) {
  Example *example = new TessellationExample("Tesselation", 800, 600);
  example->EventLoop();
  delete example;
}