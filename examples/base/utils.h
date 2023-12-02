#pragma once

#include "stb/stb_image.h"
#include <rf3d/framework/logger.h>
#include <rf3d/framework/platform.h>
#include <rf3d/framework/renderer/renderer_frontend.h>
#include <string>
#include <vector>

#if defined(PLATFORM_WINDOWS)
#define SLASH_CHAR '\\'
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_IOS) ||                      \
    defined(PLATFORM_MACOS) || defined(PLATFORM_ANDROID)
#define SLASH_CHAR '/'
#endif

class Utils {
public:
  static std::string PlatformPath(const char *path) {
    std::string result;

    for (int i = 0; i < strlen(path); ++i) {
      if (path[i] == '/' || path[i] == '\\') {
        result += SLASH_CHAR;
        continue;
      }

      result += path[i];
    }

    return result;
  }

  static void LoadTexture(GPUTexture *texture, const char *path) {
    int texture_width, texture_height, texture_num_channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &texture_width, &texture_height,
                                    &texture_num_channels, STBI_rgb_alpha);
    if (!data) {
      FATAL("Failed to load image at path %s!", path);
      return;
    }

    texture->Create(GPU_FORMAT_RGBA8, GPU_TEXTURE_TYPE_2D, texture_width,
                    texture_height);
    texture->WriteData(data, 0);

    stbi_set_flip_vertically_on_load(false);
    stbi_image_free(data);
  }

  static void LoadCubemap(GPUTexture *texture,
                          std::array<const char *, 6> paths) {
    int texture_width, texture_height, texture_num_channels;
    unsigned char *data = 0;

    for (int i = 0; i < paths.size(); ++i) {
      unsigned char *texture_data =
          stbi_load(paths[i], &texture_width, &texture_height,
                    &texture_num_channels, STBI_rgb_alpha);
      if (!texture_data) {
        FATAL("Failed to load image!");
        return;
      }

      int image_size = texture_width * texture_height * 4;

      if (!data) {
        data = (unsigned char *)malloc(sizeof(*data) * image_size * 6);
      }

      memcpy(data + image_size * i, texture_data, image_size);

      stbi_image_free(texture_data);
    }

    texture->Create(GPU_FORMAT_RGBA8, GPU_TEXTURE_TYPE_CUBEMAP, texture_width,
                    texture_height);
    texture->WriteData(data, 0);

    free(data);
  }

  static std::vector<float> GetCubeVertices() {
    std::vector<float> vertices = {
        -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.5f,  -0.5f,
        -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,
        0.0f,  -1.0f, 1.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
        1.0f,  1.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

        -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.5f,  -0.5f,
        0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,
        0.0f,  1.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        1.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,
        -0.5f, -1.0f, 0.0f,  0.0f,  1.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f,
        0.0f,  0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
        0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,
        -0.5f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,
        0.0f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
        0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.5f,  -0.5f,
        -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,
        -1.0f, 0.0f,  1.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
        1.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,

        -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,
        -0.5f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,
        1.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f};

    return vertices;
  }

  static std::vector<float> GetCubeVerticesPositionsOnly() {
    std::vector<float> vertices = {
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

    return vertices;
  }

  static std::vector<float>
  GenerateSphereVertices(float radius, int sectorCount, int stackCount) {
    std::vector<float> vertices;

    const float PI = acos(-1.0f);

    float x, y, z, xy;                           // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius; // normal
    float s, t;                                  // texCoord

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i) {
      stackAngle = PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
      xy = radius * cosf(stackAngle);      // r * cos(u)
      z = radius * sinf(stackAngle);       // r * sin(u)

      // add (sectorCount+1) vertices per stack
      // the first and last vertices have same position and normal, but
      // different tex coords
      for (int j = 0; j <= sectorCount; ++j) {
        sectorAngle = j * sectorStep; // starting from 0 to 2pi

        // vertex position
        x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
        y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        // normalized vertex normal
        nx = x * lengthInv;
        ny = y * lengthInv;
        nz = z * lengthInv;
        vertices.push_back(nx);
        vertices.push_back(ny);
        vertices.push_back(nz);

        // vertex tex coord between [0, 1]
        s = (float)j / sectorCount;
        t = (float)i / stackCount;
        vertices.push_back(s);
        vertices.push_back(t);
      }
    }

    return vertices;
  }

  static std::vector<unsigned int> GenerateSphereIndices(int sectorCount,
                                                         int stackCount) {
    std::vector<unsigned int> indices;

    // indices
    //  k1--k1+1
    //  |  / |
    //  | /  |
    //  k2--k2+1
    unsigned int k1, k2;
    for (int i = 0; i < stackCount; ++i) {
      k1 = i * (sectorCount + 1); // beginning of current stack
      k2 = k1 + sectorCount + 1;  // beginning of next stack

      for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
        // 2 triangles per sector excluding 1st and last stacks
        if (i != 0) {
          indices.push_back(k1);
          indices.push_back(k2);
          indices.push_back(k1 + 1);
        }

        if (i != (stackCount - 1)) {
          indices.push_back(k1 + 1);
          indices.push_back(k2);
          indices.push_back(k2 + 1);
        }
      }
    }

    return indices;
  }
  static std::vector<float> GenerateTerrainVertices(int rezolution, int width,
                                                    int height) {
    std::vector<float> vertices;

    for (unsigned int i = 0; i <= rezolution - 1; i++) {
      for (unsigned int j = 0; j <= rezolution - 1; j++) {
        vertices.push_back(-width / 2.0f + width * i / (float)rezolution);
        vertices.push_back(0.0f);
        vertices.push_back(-height / 2.0f + height * j / (float)rezolution);
        vertices.push_back(i / (float)rezolution);
        vertices.push_back(j / (float)rezolution);

        vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rezolution);
        vertices.push_back(0.0f);
        vertices.push_back(-height / 2.0f + height * j / (float)rezolution);
        vertices.push_back((i + 1) / (float)rezolution);
        vertices.push_back(j / (float)rezolution);

        vertices.push_back(-width / 2.0f + width * i / (float)rezolution);
        vertices.push_back(0.0f);
        vertices.push_back(-height / 2.0f +
                           height * (j + 1) / (float)rezolution);
        vertices.push_back(i / (float)rezolution);
        vertices.push_back((j + 1) / (float)rezolution);

        vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rezolution);
        vertices.push_back(0.0f);
        vertices.push_back(-height / 2.0f +
                           height * (j + 1) / (float)rezolution);
        vertices.push_back((i + 1) / (float)rezolution);
        vertices.push_back((j + 1) / (float)rezolution);
      }
    }

    return vertices;
  }
};