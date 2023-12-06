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

class TexturesExample : public Example {
public:
  TexturesExample(const char *example_name, int window_width, int window_height)
      : Example(example_name, window_width, window_height) {

    meshes.resize(3);
    for (int i = 0; i < meshes.size(); ++i) {
      meshes[i].texture = frontend->TextureAllocate();
    }

    Utils::LoadTexture(meshes[0].texture, "assets/textures/metal.png");
    meshes[0].texture->SetDebugName("Metal texture");
    meshes[0].position = glm::vec3(0, 0, 0.0f);
    Utils::LoadTexture(meshes[1].texture, "assets/textures/wood.png");
    meshes[1].texture->SetDebugName("Wood texture");
    meshes[1].position = glm::vec3(2, 0, 0.0f);
    Utils::LoadTexture(meshes[2].texture, "assets/textures/brickwall.jpg");
    meshes[2].texture->SetDebugName("Brickwall texture");
    meshes[2].position = glm::vec3(-2, 0, 0.0f);

    vertices = Utils::GetCubeVertices();
    vertex_buffer = frontend->VertexBufferAllocate();
    vertex_buffer->Create(vertices.size() * sizeof(vertices[0]));
    vertex_buffer->LoadData(0, vertices.size() * sizeof(vertices[0]),
                            vertices.data());
    vertex_buffer->SetDebugName("Cube vertex buffer");

    std::vector<GPUShaderStageConfig> stage_configs;
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/textures.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/textures.frag.spv"});

    GPUShaderConfig shader_config;
    shader_config.stage_configs = stage_configs;
    shader_config.topology_type = GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST;
    shader_config.depth_flags = GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                           GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE;
    shader_config.stencil_flags = 0;
    shader_config.render_pass = frontend->GetWindowRenderPass(); 
    shader_config.viewport_width = width;
    shader_config.viewport_height = height;

    shader = frontend->ShaderAllocate();
    shader->Create(&shader_config);
    shader->SetDebugName("Textures shader");

    global_uniform = frontend->UniformBufferAllocate();
    global_uniform->Create(sizeof(GlobalUBO));
    global_uniform->SetDebugName("Global uniform buffer");

    instance_uniform = frontend->UniformBufferAllocate();
    instance_uniform->Create(sizeof(InstanceUBO), meshes.size());
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

    for (int i = 0; i < meshes.size(); ++i) {
      meshes[i].texture_descriptor_set = frontend->DescriptorSetAllocate();
      bindings.emplace_back(GPUDescriptorBinding{
          0, GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE, meshes[i].texture, 0});
      meshes[i].texture_descriptor_set->Create(bindings);
      meshes[i].texture_descriptor_set->SetDebugName("Texture descriptor set");
      bindings.clear();
    }
  }

  virtual ~TexturesExample() {
    for (int i = 0; i < meshes.size(); ++i) {
      meshes[i].texture->Destroy();
      delete meshes[i].texture;
      meshes[i].texture_descriptor_set->Destroy();
      delete meshes[i].texture_descriptor_set;
    }

    instance_uniform->Destroy();
    delete instance_uniform;
    global_uniform->Destroy();
    delete global_uniform;
    global_descriptor_set->Destroy();
    delete global_descriptor_set;
    instance_descriptor_set->Destroy();
    delete instance_descriptor_set;

    shader->Destroy();
    delete shader;
    vertex_buffer->Destroy();
    delete vertex_buffer;
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

        shader->Bind();
        vertex_buffer->Bind(0);
        shader->BindUniformBuffer(global_descriptor_set, 0, 0);

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
                                    1);

          shader->BindSampler(meshes[i].texture_descriptor_set, 2);

          frontend->Draw(vertices.size() / 5);
        }

        frontend->EndDebugRegion();
        frontend->GetWindowRenderPass()->End();

        frontend->EndFrame();
      }

      UpdateEnd();
    }
  }

private:
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

  std::vector<MeshParams> meshes;

  std::vector<float> vertices;
  GPUVertexBuffer *vertex_buffer;

  GPUShader *shader;

  GPUUniformBuffer *global_uniform;
  GPUUniformBuffer *instance_uniform;
  GPUDescriptorSet *global_descriptor_set;
  GPUDescriptorSet *instance_descriptor_set;
};

int main(int argc, char **argv) {
  Example *example = new TexturesExample("Textures", 800, 600);
  example->EventLoop();
  delete example;
}