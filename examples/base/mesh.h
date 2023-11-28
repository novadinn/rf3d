#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <rf3d/framework/renderer/renderer_frontend.h>
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

struct MeshRequiredFormat {
  bool positions;
  bool normals;
  bool texture_coordinates;
  bool tangents_and_bitangents;
};

#define MESH_TEXTURE_DIFFUSE_INDEX 0
#define MESH_TEXTURE_SPECULAR_INDEX 1
#define MESH_TEXTURE_NORMAL_INDEX 2

struct Mesh {
  std::vector<float> vertices;
  std::vector<uint32_t> indices;
  std::string texture_paths[3];
  bool textured;
};

class MeshLoader {
public:
  static std::vector<Mesh> Load(MeshRequiredFormat *required_format,
                                const char *file_path);

private:
  static std::vector<Mesh> ProcessNode(MeshRequiredFormat *required_format,
                                       aiNode *node, const aiScene *scene);
  static Mesh ProcessMesh(MeshRequiredFormat *required_format, aiMesh *mesh,
                          const aiScene *scene);
};