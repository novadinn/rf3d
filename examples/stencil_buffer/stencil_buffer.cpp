#include <iostream>

#include "../base/example.h"
#include "../base/utils.h"
#include "../base/mesh.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rf3d/framework/logger.h>
#include <rf3d/framework/renderer/renderer_frontend.h>

class StencilBuffer : public Example {
public:
  StencilBuffer(const char *example_name, int window_width, int window_height)
      : Example(example_name, window_width, window_height) {
    MeshRequiredFormat format = {true, true, true, false};
    sia_meshes = MeshLoader::Load(&format, "assets/models/sia.obj");

    for (int i = 0; i < sia_meshes.size(); ++i) {
      GPUVertexBuffer *vertex_buffer = frontend->VertexBufferAllocate();
      vertex_buffer->Create(sia_meshes[i].vertices.size() *
                            sizeof(sia_meshes[i].vertices[0]));
      vertex_buffer->LoadData(0,
                              sia_meshes[i].vertices.size() *
                                  sizeof(sia_meshes[i].vertices[0]),
                              sia_meshes[i].vertices.data());
      vertex_buffer->SetDebugName("Sia vertex buffer");

      vertex_buffers.emplace_back(vertex_buffer);

      GPUIndexBuffer *index_buffer = frontend->IndexBufferAllocate();
      index_buffer->Create(sia_meshes[i].indices.size() *
                           sizeof(sia_meshes[i].indices[0]));
      index_buffer->LoadData(0,
                             sia_meshes[i].indices.size() *
                                 sizeof(sia_meshes[i].indices[0]),
                             sia_meshes[i].indices.data());
      index_buffer->SetDebugName("Sia index buffer");

      index_buffers.emplace_back(index_buffer);
    }

    std::vector<GPUShaderStageConfig> stage_configs;
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/toon.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/toon.frag.spv"});

    GPUShaderConfig shader_config;
    shader_config.stage_configs = stage_configs;
    shader_config.topology_type = GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST;
    shader_config.depth_flags = GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                           GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE;
    shader_config.stencil_flags = 0;
    shader_config.render_pass = frontend->GetWindowRenderPass(); 
    shader_config.viewport_width = width;
    shader_config.viewport_height = height;

    toon_shader = frontend->ShaderAllocate();
    toon_shader->Create(&shader_config);
    toon_shader->SetDebugName("Toon shader");

    stage_configs.clear();
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/outline.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/outline.frag.spv"});
    shader_config.stage_configs = stage_configs;

    outline_shader = frontend->ShaderAllocate();
    outline_shader->Create(&shader_config);
    outline_shader->SetDebugName("Outline shader");

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
  }

  virtual ~StencilBuffer() {
    global_descriptor_set->Destroy();
    delete global_descriptor_set;
    instance_descriptor_set->Destroy();
    delete instance_descriptor_set;
    instance_uniform->Destroy();
    delete instance_uniform;
    global_uniform->Destroy();
    delete global_uniform;
    toon_shader->Destroy();
    delete toon_shader;
    outline_shader->Destroy();
    delete outline_shader;
    for(int i = 0; i < sia_meshes.size(); ++i) {
        vertex_buffers[i]->Destroy();
        delete vertex_buffers[i];
        index_buffers[i]->Destroy();
        delete index_buffers[i];
    }
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
        instance_ubo.model =
            glm::translate(instance_ubo.model, glm::vec3(0.0, -1.0, 0.0));
        instance_uniform->LoadData(0, instance_uniform->GetSize(),
                                     &instance_ubo);

        for(int i = 0; i < sia_meshes.size(); ++i) {
            GPUVertexBuffer *vertex_buffer = vertex_buffers[i];
            GPUIndexBuffer *index_buffer = index_buffers[i];

            toon_shader->Bind();
            vertex_buffer->Bind(0);
            index_buffer->Bind(0);
            toon_shader->BindUniformBuffer(global_descriptor_set, 0, 0);
            toon_shader->BindUniformBuffer(instance_descriptor_set, 0, 1);

            frontend->DrawIndexed(sia_meshes[i].indices.size());

            outline_shader->Bind();
            vertex_buffer->Bind(0);
            index_buffer->Bind(0);
            outline_shader->BindUniformBuffer(global_descriptor_set, 0, 0);
            outline_shader->BindUniformBuffer(instance_descriptor_set, 0, 1);

            frontend->DrawIndexed(sia_meshes[i].indices.size());
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
    glm::vec3 position;
  };
  struct GlobalUBO {
    glm::mat4 view;
    glm::mat4 projection;
  };
  struct InstanceUBO {
    glm::mat4 model;
  };

  std::vector<Mesh> sia_meshes;
  std::vector<GPUVertexBuffer *> vertex_buffers;
  std::vector<GPUIndexBuffer *> index_buffers;

  GPUShader *toon_shader;
  GPUShader *outline_shader;

  GPUUniformBuffer *global_uniform;
  GPUUniformBuffer *instance_uniform;
  GPUDescriptorSet *global_descriptor_set;
  GPUDescriptorSet *instance_descriptor_set;
};

int main(int argc, char **argv) {
  Example *example = new StencilBuffer("Stencil buffer", 800, 600);
  example->EventLoop();
  delete example;
}