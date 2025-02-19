#pragma once

#include <array>

#include "glad/glad.h"
#include <glm/glm.hpp>

#include "intersection_utils/bvh.h"

namespace AssetUtils {
struct Material {
  glm::vec3 diffuse;
  glm::vec3 specular;
  float specular_ex;
  GLuint texture;
  bool use_texture = false;
};

struct Model {
  IntersectionUtils::BVH model_bvh; // most likely should exist on GPU and this should be GLuint
  std::vector<Material> model_materials;
}

namespace Detail {
struct Face {
  std::array<std::uint32_t, 3> vertex_idxs;
  std::array<std::uint32_t, 3> texture_idxs;
  std::array<std::uint32_t, 3> normal_idxs;
  std::uint8_t valid_idxs = 0;

  void SetVertexIdxsValid() { valid_idxs |= 1; }
  void SetTextureIdxsValid() { valid_idxs |= 1 << 1; }
  void SetNormalIdxsValid() { valid_idxs |= 1 << 2; }
  bool IsVertexIdxsValid() { return valid_idxs & 1; }
  bool IsTextureIdxsValid() { return valid_idxs & (1 << 1); }
  bool IsNormalIdxsValid() { return valid_idxs & (1 << 2); }
}

struct CpuSubGeometry {
  std::string material;
  std::vector<Face> faces;
}

struct CpuGeometry {
  std::string object_id;
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> textures;
  std::vector<glm::vec3> normals;
  std::vector<CpuSubGeometry> geometries;
  bool has_normals = false;
  bool has_texcoords = false;
}

}
}  // namespace AssetUtils

