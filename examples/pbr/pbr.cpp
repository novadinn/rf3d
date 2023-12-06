#include <iostream>

#include "../base/example.h"
#include "../base/utils.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rf3d/framework/logger.h>
#include <rf3d/framework/renderer/renderer_frontend.h>

class PBRExample : public Example {
public:
  PBRExample(const char *example_name, int window_width, int window_height)
      : Example(example_name, window_width, window_height) {
    meshes.resize(3);
    meshes[0].position = glm::vec3(0, 0, 0.0f);
    meshes[0].push_constants =
        PushConsts{0.1f, 1.0f, 0.672411f, 0.637331f, 0.585456f};
    meshes[1].position = glm::vec3(2, 0, 0.0f);
    meshes[1].push_constants = PushConsts{0.8f, 0.2f, 1, 0, 0};
    meshes[2].position = glm::vec3(-2, 0, 0.0f);
    meshes[2].push_constants = PushConsts{0.5f, 0.5f, 0, 1, 0};

    vertices = Utils::GetCubeVertices();

    vertex_buffer = frontend->VertexBufferAllocate();
    vertex_buffer->Create(vertices.size() * sizeof(vertices[0]));
    vertex_buffer->LoadData(0, vertices.size() * sizeof(vertices[0]),
                            vertices.data());
    vertex_buffer->SetDebugName("Cube vertex buffer");

    std::vector<GPUShaderStageConfig> stage_configs;
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/pbr.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/pbr.frag.spv"});

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
    shader->SetDebugName("PBR shader");

    global_uniform = frontend->UniformBufferAllocate();
    global_uniform->Create(sizeof(GlobalUBO));
    global_uniform->SetDebugName("Global uniform buffer");

    world_uniform = frontend->UniformBufferAllocate();
    world_uniform->Create(sizeof(WorldUBO));
    world_uniform->SetDebugName("World uniform buffer");

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

    world_descriptor_set = frontend->DescriptorSetAllocate();
    bindings.emplace_back(GPUDescriptorBinding{
        0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, world_uniform});
    world_descriptor_set->Create(bindings);
    world_descriptor_set->SetDebugName("World descriptor set");
    bindings.clear();

    instance_descriptor_set = frontend->DescriptorSetAllocate();
    bindings.emplace_back(GPUDescriptorBinding{
        0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, instance_uniform});
    instance_descriptor_set->Create(bindings);
    instance_descriptor_set->SetDebugName("Instance descriptor set");
    bindings.clear();
  }

  virtual ~PBRExample() {
    global_descriptor_set->Destroy();
    delete global_descriptor_set;
    world_descriptor_set->Destroy();
    delete world_descriptor_set;
    instance_descriptor_set->Destroy();
    delete instance_descriptor_set;
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

        frontend->EndDebugRegion();
        frontend->GetWindowRenderPass()->End();

        frontend->EndFrame();
      }

      UpdateEnd();
    }
  }

private:
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

  std::vector<MeshParams> meshes;

  std::vector<float> vertices;
  GPUVertexBuffer *vertex_buffer;

  GPUShader *shader;

  GPUUniformBuffer *global_uniform;
  GPUUniformBuffer *world_uniform;
  GPUUniformBuffer *instance_uniform;
  GPUDescriptorSet *global_descriptor_set;
  GPUDescriptorSet *world_descriptor_set;
  GPUDescriptorSet *instance_descriptor_set;
};

int main(int argc, char **argv) {
  Example *example = new PBRExample("PBR", 800, 600);
  example->EventLoop();
  delete example;
}