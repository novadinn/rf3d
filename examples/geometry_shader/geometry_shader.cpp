#include <iostream>

#include "../base/example.h"
#include "../base/utils.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rf3d/framework/logger.h>
#include <rf3d/framework/renderer/renderer_frontend.h>

class GeometryShaderExample : public Example {
public:
  GeometryShaderExample(const char *example_name, int window_width,
                        int window_height)
      : Example(example_name, window_width, window_height) {

    global_uniform = frontend->UniformBufferAllocate();
    global_uniform->Create(sizeof(GlobalUBO));
    global_uniform->SetDebugName("Global uniform");
    instance_uniform = frontend->UniformBufferAllocate();
    instance_uniform->Create(sizeof(InstanceUBO));
    instance_uniform->SetDebugName("Instance uniform");

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

    vertices = Utils::GenerateSphereVertices(1, 36, 18);
    indices = Utils::GenerateSphereIndices(36, 18);

    vertex_buffer = frontend->VertexBufferAllocate();
    vertex_buffer->Create(vertices.size() * sizeof(vertices[0]));
    vertex_buffer->LoadData(0, vertices.size() * sizeof(vertices[0]),
                            vertices.data());
    vertex_buffer->SetDebugName("Sphere vertex buffer");

    index_buffer = frontend->IndexBufferAllocate();
    index_buffer->Create(indices.size() * sizeof(indices[0]));
    index_buffer->LoadData(0, indices.size() * sizeof(indices[0]),
                           indices.data());
    index_buffer->SetDebugName("Sphere index buffer");

    std::vector<GPUShaderStageConfig> stage_configs;
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/normal.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_GEOMETRY, "assets/shaders/normal.geom.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/normal.frag.spv"});

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
    shader->SetDebugName("Normals debug shader");
  }

  virtual ~GeometryShaderExample() {
    vertex_buffer->Destroy();
    delete vertex_buffer;

    index_buffer->Destroy();
    delete index_buffer;

    shader->Destroy();
    delete shader;

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

        InstanceUBO instance_ubo = {};
        instance_ubo.model = glm::mat4(1.0f);
        instance_ubo.model =
            glm::translate(instance_ubo.model, glm::vec3(0.0f));
        instance_uniform->LoadData(0, sizeof(InstanceUBO), &instance_ubo);

        shader->Bind();
        vertex_buffer->Bind(0);
        index_buffer->Bind(0);
        shader->BindUniformBuffer(global_descriptor_set, 0, 0);
        shader->BindUniformBuffer(instance_descriptor_set, 0, 1);
        frontend->DrawIndexed(indices.size());

        frontend->GetWindowRenderPass()->End();

        frontend->EndDebugRegion();
        frontend->EndFrame();
      }
    }

    UpdateEnd();
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

  std::vector<float> vertices;
  std::vector<unsigned int> indices;

  GPUVertexBuffer *vertex_buffer;
  GPUIndexBuffer *index_buffer;

  GPUShader *shader;
};

int main(int argc, char **argv) {
  Example *example = new GeometryShaderExample("Geometry Shader", 800, 600);
  example->EventLoop();
  delete example;
}