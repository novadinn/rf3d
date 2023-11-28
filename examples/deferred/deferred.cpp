#include <iostream>

#include "../base/example.h"
#include "../base/mesh.h"
#include "../base/utils.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <rf3d/framework/logger.h>
#include <rf3d/framework/renderer/renderer_frontend.h>
#include <unordered_map>

class DeferredExample : public Example {
public:
  DeferredExample(const char *example_name, int window_width, int window_height)
      : Example(example_name, window_width, window_height) {
    MeshRequiredFormat format = {true, true, true, true};
    sponza_scene = MeshLoader::Load(&format, "assets/models/sponza.obj");

    for (int i = 0; i < sponza_scene.size(); ++i) {
      GPUVertexBuffer *vertex_buffer = frontend->VertexBufferAllocate();
      vertex_buffer->Create(sponza_scene[i].vertices.size() *
                            sizeof(sponza_scene[i].vertices[0]));
      vertex_buffer->LoadData(0,
                              sponza_scene[i].vertices.size() *
                                  sizeof(sponza_scene[i].vertices[0]),
                              sponza_scene[i].vertices.data());
      vertex_buffer->SetDebugName("MRT vertex buffer");

      sponza_vertex_buffers.emplace_back(vertex_buffer);

      GPUIndexBuffer *index_buffer = frontend->IndexBufferAllocate();
      index_buffer->Create(sponza_scene[i].indices.size() *
                           sizeof(sponza_scene[i].indices[0]));
      index_buffer->LoadData(0,
                             sponza_scene[i].indices.size() *
                                 sizeof(sponza_scene[i].indices[0]),
                             sponza_scene[i].indices.data());
      index_buffer->SetDebugName("MRT index buffer");

      sponza_index_buffers.emplace_back(index_buffer);

      if (sponza_scene[i].textured) {
        const std::string texture_assets_path = "assets/";

        std::string current_texture_path =
            texture_assets_path +
            sponza_scene[i].texture_paths[MESH_TEXTURE_DIFFUSE_INDEX];
        GPUTexture *diffuse_texture;
        if (sponza_texture_cache.count(current_texture_path) == 0) {
          diffuse_texture = frontend->TextureAllocate();
          Utils::LoadTexture(
              diffuse_texture,
              Utils::PlatformPath(current_texture_path.c_str()).c_str());

          sponza_texture_cache.emplace(current_texture_path, diffuse_texture);
        } else {
          diffuse_texture = sponza_texture_cache.at(current_texture_path);
        }
        sponza_diffuse_textures.emplace_back(diffuse_texture);

        current_texture_path =
            texture_assets_path +
            sponza_scene[i].texture_paths[MESH_TEXTURE_SPECULAR_INDEX];
        GPUTexture *specular_texture;
        if (sponza_texture_cache.count(current_texture_path) == 0) {
          specular_texture = frontend->TextureAllocate();
          Utils::LoadTexture(
              specular_texture,
              Utils::PlatformPath(current_texture_path.c_str()).c_str());

          sponza_texture_cache.emplace(current_texture_path, specular_texture);
        } else {
          specular_texture = sponza_texture_cache.at(current_texture_path);
        }
        sponza_specular_textures.emplace_back(specular_texture);

        current_texture_path =
            texture_assets_path +
            sponza_scene[i].texture_paths[MESH_TEXTURE_NORMAL_INDEX];
        GPUTexture *normal_texture;
        if (sponza_texture_cache.count(current_texture_path) == 0) {
          normal_texture = frontend->TextureAllocate();
          Utils::LoadTexture(
              normal_texture,
              Utils::PlatformPath(current_texture_path.c_str()).c_str());

          sponza_texture_cache.emplace(current_texture_path, normal_texture);
        } else {
          normal_texture = sponza_texture_cache.at(current_texture_path);
        }
        sponza_normal_textures.emplace_back(normal_texture);
      } else {
        GPUTexture *default_texture = frontend->TextureAllocate();
        Utils::LoadTexture(default_texture, "assets/textures/metal.png");
        sponza_diffuse_textures.emplace_back(default_texture);
        sponza_specular_textures.emplace_back(default_texture);
        sponza_normal_textures.emplace_back(default_texture);
        sponza_texture_cache.emplace("assets/textures/metal.png",
                                     default_texture);
      }
    }

    std::vector<GPUShaderStageConfig> stage_configs;
    stage_configs.clear();
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_VERTEX, "assets/shaders/mrt.vert.spv"});
    stage_configs.emplace_back(GPUShaderStageConfig{
        GPU_SHADER_STAGE_TYPE_FRAGMENT, "assets/shaders/mrt.frag.spv"});
    mrt_shader = frontend->ShaderAllocate();
    mrt_shader->Create(stage_configs, GPU_SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST,
                       GPU_SHADER_DEPTH_FLAG_DEPTH_TEST_ENABLE |
                           GPU_SHADER_DEPTH_FLAG_DEPTH_WRITE_ENABLE,
                       frontend->GetWindowRenderPass(), width, height);
    mrt_shader->SetDebugName("MRT shader");

    mrt_global_uniform = frontend->UniformBufferAllocate();
    mrt_global_uniform->Create(sizeof(GlobalUBO));
    mrt_global_uniform->SetDebugName("Global uniform buffer");

    mrt_instance_uniform = frontend->UniformBufferAllocate();
    mrt_instance_uniform->Create(sizeof(InstanceUBO));
    mrt_instance_uniform->SetDebugName("Instance uniform buffer");

    std::vector<GPUDescriptorBinding> bindings;
    bindings.clear();
    bindings.emplace_back(GPUDescriptorBinding{
        0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0, mrt_global_uniform});
    mrt_global_descriptor_set = frontend->DescriptorSetAllocate();
    mrt_global_descriptor_set->Create(bindings);
    mrt_global_descriptor_set->SetDebugName("Global descriptor set");

    bindings.clear();
    bindings.emplace_back(
        GPUDescriptorBinding{0, GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER, 0,
                             mrt_instance_uniform});
    mrt_instance_descriptor_set = frontend->DescriptorSetAllocate();
    mrt_instance_descriptor_set->Create(bindings);
    mrt_instance_descriptor_set->SetDebugName("Instance descriptor set");

    for (int i = 0; i < sponza_scene.size(); ++i) {
      GPUDescriptorSet *texture_descriptor_set;

      bindings.clear();
      bindings.emplace_back(
          GPUDescriptorBinding{0, GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE,
                               sponza_diffuse_textures[i], 0, 0});
      bindings.emplace_back(
          GPUDescriptorBinding{1, GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE,
                               sponza_specular_textures[i], 0, 0});
      bindings.emplace_back(
          GPUDescriptorBinding{2, GPU_DESCRIPTOR_BINDING_TYPE_TEXTURE,
                               sponza_normal_textures[i], 0, 0});
      texture_descriptor_set = frontend->DescriptorSetAllocate();
      texture_descriptor_set->Create(bindings);
      texture_descriptor_set->SetDebugName("Texture descriptor set");

      mtr_texture_descriptor_sets.emplace_back(texture_descriptor_set);
    }
  }

  virtual ~DeferredExample() {
    mrt_instance_descriptor_set->Destroy();
    delete mrt_instance_descriptor_set;

    mrt_global_descriptor_set->Destroy();
    delete mrt_global_descriptor_set;

    mrt_instance_uniform->Destroy();
    delete mrt_instance_uniform;

    mrt_global_uniform->Destroy();
    delete mrt_global_uniform;

    mrt_shader->Destroy();
    delete mrt_shader;

    for (auto it = sponza_texture_cache.begin();
         it != sponza_texture_cache.end(); ++it) {
      it->second->Destroy();
      delete it->second;
    }

    for (int i = 0; i < sponza_scene.size(); ++i) {
      sponza_vertex_buffers[i]->Destroy();
      delete sponza_vertex_buffers[i];

      sponza_index_buffers[i]->Destroy();
      delete sponza_index_buffers[i];

      mtr_texture_descriptor_sets[i]->Destroy();
      delete mtr_texture_descriptor_sets[i];
    }
  }

  void EventLoop() override {
    while (running) {
      UpdateStart();

      if (frontend->BeginFrame()) {
        frontend->GetWindowRenderPass()->Begin(
            frontend->GetCurrentWindowRenderTarget());
        frontend->BeginDebugRegion("Offscreen pass",
                                   glm::vec4(1.0, 0.0, 0.0, 1.0));

        glm::vec3 camera_position = glm::vec3(0, 0, 0.0f);
        GlobalUBO global_ubo = {};
        global_ubo.view = camera->GetViewMatrix();
        global_ubo.projection = camera->GetProjectionMatrix();
        mrt_global_uniform->LoadData(0, mrt_global_uniform->GetSize(),
                                     &global_ubo);

        InstanceUBO instance_ubo = {};
        instance_ubo.model = glm::mat4(1.0f);
        instance_ubo.model =
            glm::translate(instance_ubo.model, glm::vec3(0.0f));
        mrt_instance_uniform->LoadData(0, sizeof(InstanceUBO), &instance_ubo);

        mrt_shader->Bind();
        mrt_shader->BindUniformBuffer(mrt_global_descriptor_set, 0, 0);
        mrt_shader->BindUniformBuffer(mrt_instance_descriptor_set, 0, 1);
        for (int i = 0; i < sponza_scene.size(); ++i) {
          sponza_index_buffers[i]->Bind(0);
          sponza_vertex_buffers[i]->Bind(0);
          mrt_shader->BindSampler(mtr_texture_descriptor_sets[i], 2);
          frontend->DrawIndexed(sponza_scene[i].indices.size());
        }

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

  std::vector<Mesh> sponza_scene;
  /* TODO: just use 1 giant vertex and index buffers */
  std::vector<GPUVertexBuffer *> sponza_vertex_buffers;
  std::vector<GPUIndexBuffer *> sponza_index_buffers;
  std::unordered_map<std::string, GPUTexture *> sponza_texture_cache;
  std::vector<GPUTexture *> sponza_diffuse_textures;
  std::vector<GPUTexture *> sponza_specular_textures;
  std::vector<GPUTexture *> sponza_normal_textures;

  GPUShader *mrt_shader;

  GPUUniformBuffer *mrt_global_uniform;
  GPUUniformBuffer *mrt_instance_uniform;
  GPUDescriptorSet *mrt_global_descriptor_set;
  GPUDescriptorSet *mrt_instance_descriptor_set;
  std::vector<GPUDescriptorSet *> mtr_texture_descriptor_sets;
};

int main(int argc, char **argv) {
  Example *example = new DeferredExample("Deferred", 800, 600);
  example->EventLoop();
  delete example;
}