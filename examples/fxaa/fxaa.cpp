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

class FXAAExamle : public Example {
public:
  FXAAExamle(const char *example_name, int window_width, int window_height)
      : Example(example_name, window_width, window_height) {

    vertices = Utils::GetCubeVertices();

    vertex_buffer = frontend->VertexBufferAllocate();
    vertex_buffer->Create(vertices.size() * sizeof(vertices[0]));
    vertex_buffer->LoadData(0, vertices.size() * sizeof(vertices[0]),
                            vertices.data());
    vertex_buffer->SetDebugName("Cube vertex buffer");

    meshes.resize(3);
    for (int i = 0; i < meshes.size(); ++i) {
      meshes[i].texture = frontend->TextureAllocate();
    }

    Utils::LoadTexture(meshes[0].texture, "assets/textures/metal.png");
    meshes[0].texture->SetDebugName("Metal texture");
    meshes[0].position = glm::vec3(0, 0, 5.0f);
    Utils::LoadTexture(meshes[1].texture, "assets/textures/wood.png");
    meshes[1].texture->SetDebugName("Wood texture");
    meshes[1].position = glm::vec3(2, 0, 5.0f);
    Utils::LoadTexture(meshes[2].texture, "assets/textures/brickwall.jpg");
    meshes[2].texture->SetDebugName("Brickwall texture");
    meshes[2].position = glm::vec3(-2, 0, 5.0f);

    offscreen_color_attachment = frontend->AttachmentAllocate();
    offscreen_color_attachment->Create(GPU_FORMAT_DEVICE_COLOR_OPTIMAL,
                                       GPU_ATTACHMENT_USAGE_COLOR_ATTACHMENT,
                                       width, height);
    offscreen_color_attachment->SetDebugName("Offscreen color attachment");
    offscreen_depth_attachment = frontend->AttachmentAllocate();
    offscreen_depth_attachment->Create(
        GPU_FORMAT_DEVICE_DEPTH_OPTIMAL,
        GPU_ATTACHMENT_USAGE_DEPTH_STENCIL_ATTACHMENT, width, height);
    offscreen_depth_attachment->SetDebugName("Offscreen depth attachment");

    offscreen_render_pass = frontend->RenderPassAllocate();
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
    offscreen_render_pass->SetDebugName("Offscreen render pass");
    offscreen_render_target = frontend->RenderTargetAllocate();
    offscreen_render_target->Create(
        offscreen_render_pass,
        std::vector<GPUAttachment *>{offscreen_color_attachment,
                                     offscreen_depth_attachment},
        width, height);
    offscreen_render_target->SetDebugName("Offscreen framebuffer");

    shader = frontend->ShaderAllocate();
    std::vector<GPUShaderStageConfig> stage_configs;
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/textures.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/textures.frag.spv"});
    shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                   GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                       GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                   offscreen_render_pass, width, height);
    shader->SetDebugName("Texture shader");

    global_uniform = frontend->UniformBufferAllocate();
    global_uniform->Create(sizeof(GlobalUBO));
    global_uniform->SetDebugName("Global uniform buffer");
    instance_uniform = frontend->UniformBufferAllocate();
    instance_uniform->Create(sizeof(InstanceUBO), meshes.size());
    global_uniform->SetDebugName("Instance uniform buffer");

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

    post_processing_shader = frontend->ShaderAllocate();
    stage_configs.clear();
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/fxaa.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/fxaa.frag.spv"});
    post_processing_shader->Create(
        stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
        GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
            GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
        frontend->GetWindowRenderPass(), width, height);
    post_processing_shader->SetDebugName("FXAA shader");

    post_processing_set = frontend->DescriptorSetAllocate();
    bindings.clear();
    bindings.emplace_back(
        GPUDescriptorBinding{0, GPU_DESCRIPTOR_BINDING_TYPE_ATTACHMENT, 0, 0,
                             offscreen_color_attachment});
    post_processing_set->Create(bindings);
    post_processing_set->SetDebugName("Post processing descriptor set");
  }
  virtual ~FXAAExamle() {
    post_processing_set->Destroy();
    delete post_processing_set;
    post_processing_shader->Destroy();
    delete post_processing_shader;

    offscreen_render_target->Destroy();
    offscreen_depth_attachment->Destroy();
    offscreen_color_attachment->Destroy();
    offscreen_render_pass->Destroy();
    delete offscreen_render_pass;
    for (int i = 0; i < meshes.size(); ++i) {
      meshes[i].texture->Destroy();
      delete meshes[i].texture;
    }
    global_descriptor_set->Destroy();
    delete global_descriptor_set;
    instance_descriptor_set->Destroy();
    delete instance_descriptor_set;
    instance_uniform->Destroy();
    delete instance_uniform;
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
        offscreen_render_pass->Begin(offscreen_render_target);
        frontend->BeginDebugRegion("Offscreen pass",
                                   glm::vec4(1.0, 0.0, 0.0, 1.0));

        static float angle = 0.0f;
        angle += 0.003f;

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
                glm::translate(instance_ubo.model, meshes[j].position) *
                glm::rotate(instance_ubo.model, angle,
                            glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f)));

            instance_ubos.emplace_back(instance_ubo);
          }

          instance_uniform->LoadData(0, instance_uniform->GetSize(),
                                     instance_ubos.data());
          shader->BindUniformBuffer(instance_descriptor_set,
                                    i * instance_uniform->GetDynamicAlignment(),
                                    1);

          shader->BindSampler(meshes[i].texture_descriptor_set, 2);

          frontend->Draw(vertices.size() / 8);
        }

        frontend->EndDebugRegion();
        offscreen_render_pass->End();

        frontend->GetWindowRenderPass()->Begin(
            frontend->GetCurrentWindowRenderTarget());
        frontend->BeginDebugRegion("Main pass", glm::vec4(0.0, 1.0, 0.0, 1.0));

        post_processing_shader->Bind();
        post_processing_shader->BindSampler(post_processing_set, 0);
        frontend->Draw(4);

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

  GPUVertexBuffer *vertex_buffer;
  std::vector<float> vertices;
  GPUShader *shader;
  std::vector<MeshParams> meshes;

  GPUAttachment *offscreen_color_attachment;
  GPUAttachment *offscreen_depth_attachment;
  GPURenderPass *offscreen_render_pass;
  GPURenderTarget *offscreen_render_target;

  GPUUniformBuffer *global_uniform;
  GPUUniformBuffer *instance_uniform;
  GPUDescriptorSet *global_descriptor_set;
  GPUDescriptorSet *instance_descriptor_set;

  GPUShader *post_processing_shader;
  GPUDescriptorSet *post_processing_set;
};

int main(int argc, char **argv) {
  Example *example = new FXAAExamle("FXAA", 800, 600);
  example->EventLoop();
  delete example;
}