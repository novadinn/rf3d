#include "mesh.h"

#include <rf3d/framework/logger.h>

std::vector<Mesh> MeshLoader::Load(MeshRequiredFormat *required_format,
                                   const char *file_path) {
  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile(file_path, aiProcessPreset_TargetRealtime_MaxQuality);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    FATAL("Error occurred while loading model at path %s: %s", file_path,
          importer.GetErrorString());
    return std::vector<Mesh>{};
  }

  return ProcessNode(required_format, scene->mRootNode, scene);
}

std::vector<Mesh> MeshLoader::ProcessNode(MeshRequiredFormat *required_format,
                                          aiNode *node, const aiScene *scene) {
  std::vector<Mesh> meshes;
  for (int i = 0; i < node->mNumMeshes; ++i) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.emplace_back(ProcessMesh(required_format, mesh, scene));
  }
  for (int i = 0; i < node->mNumChildren; ++i) {
    std::vector<Mesh> node_meshes =
        ProcessNode(required_format, node->mChildren[i], scene);

    for (int j = 0; j < node_meshes.size(); ++j) {
      meshes.emplace_back(node_meshes[j]);
    }
  }

  return meshes;
}

Mesh MeshLoader::ProcessMesh(MeshRequiredFormat *required_format, aiMesh *mesh,
                             const aiScene *scene) {
  Mesh result;

  if ((required_format->positions && !mesh->HasPositions()) ||
      (required_format->normals && !mesh->HasNormals()) ||
      (required_format->texture_coordinates && !mesh->HasTextureCoords(0)) ||
      (required_format->tangents_and_bitangents &&
       !mesh->HasTangentsAndBitangents())) {
    WARN("Mesh format is not the same as required format!");
  }

  for (int i = 0; i < mesh->mNumVertices; ++i) {
    if (required_format->positions && mesh->HasPositions()) {
      result.vertices.emplace_back(mesh->mVertices[i].x);
      result.vertices.emplace_back(mesh->mVertices[i].y);
      result.vertices.emplace_back(mesh->mVertices[i].z);
    }

    if (required_format->normals && mesh->HasNormals()) {
      result.vertices.emplace_back(mesh->mNormals[i].x);
      result.vertices.emplace_back(mesh->mNormals[i].y);
      result.vertices.emplace_back(mesh->mNormals[i].z);
    }

    if (required_format->texture_coordinates && mesh->HasTextureCoords(0)) {
      result.vertices.emplace_back(mesh->mTextureCoords[0][i].x);
      result.vertices.emplace_back(mesh->mTextureCoords[0][i].y);
    }

    if (required_format->tangents_and_bitangents &&
        mesh->HasTangentsAndBitangents()) {
      result.vertices.emplace_back(mesh->mTangents[i].x);
      result.vertices.emplace_back(mesh->mTangents[i].y);
      result.vertices.emplace_back(mesh->mTangents[i].z);

      result.vertices.emplace_back(mesh->mBitangents[i].x);
      result.vertices.emplace_back(mesh->mBitangents[i].y);
      result.vertices.emplace_back(mesh->mBitangents[i].z);
    }
  }

  for (int i = 0; i < mesh->mNumFaces; ++i) {
    aiFace face = mesh->mFaces[i];
    for (int j = 0; j < face.mNumIndices; ++j) {
      result.indices.emplace_back(face.mIndices[j]);
    }
  }

  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
  result.textured = false;
  if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
    aiString texture_name;
    material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_name);
    result.texture_paths[MESH_TEXTURE_DIFFUSE_INDEX] = texture_name.C_Str();
    result.textured = true;
  }
  if (material->GetTextureCount(aiTextureType_SPECULAR)) {
    aiString texture_name;
    material->GetTexture(aiTextureType_SPECULAR, 0, &texture_name);
    result.texture_paths[MESH_TEXTURE_SPECULAR_INDEX] = texture_name.C_Str();
    result.textured = true;
  }
  if (material->GetTextureCount(aiTextureType_HEIGHT)) {
    aiString texture_name;
    material->GetTexture(aiTextureType_HEIGHT, 0, &texture_name);
    result.texture_paths[MESH_TEXTURE_NORMAL_INDEX] = texture_name.C_Str();
    result.textured = true;
  }

  return result;
}
