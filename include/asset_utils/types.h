#pragma once

#include <array>
#include <optional>
#include <type_traits>
#include <utility>

#include "glad/glad.h"
#include <glm/glm.hpp>

#include "intersection_utils/bvh.h"
#include "common/types.h"
#include "asset_utils/gpu_texture.h"

namespace AssetUtils {
namespace GPU {
struct PackedVertexData {
  alignas(16) glm::vec3 vertex;
  alignas(8) glm::vec2 texture;

  PackedVertexData(const glm::vec3& v, const glm::vec2& t)
    : vertex(v), texture(t) {}
};

struct Triangle {
  std::array<std::uint32_t, 3> vertex_idxs;
  std::uint32_t material_idx;
};
}

struct Material {
  glm::vec3 diffuse;
  glm::vec3 specular;
  float specular_ex;
  GPUTexture texture;
  bool use_texture = false;
};

struct Model {
  IntersectionUtils::BVH<GPU::Triangle> model_bvh;
  std::vector<Material> model_materials;
  std::vector<GPU::PackedVertexData> vertex_data_buffer;

  Model(
    IntersectionUtils::BVH<GPU::Triangle> _model_bvh,
    std::vector<Material> _model_materials,
    std::vector<GPU::PackedVertexData> _vertex_data_buffer)
    : model_bvh(std::move(_model_bvh)),
      model_materials(std::move(_model_materials)),
      vertex_data_buffer(std::move(_vertex_data_buffer))
   {};
};

// These are temporary structs for creating the above
namespace Detail {
struct Face {
  std::array<std::uint32_t, 3> vertex_idxs;
  std::array<std::uint32_t, 3> texture_idxs;
  std::array<std::uint32_t, 3> normal_idxs;
  std::uint8_t valid_idxs = 0;

  template <class... Ts>
  Face(Ts... nums) {
    static_assert((std::is_same_v<Ts, std::uint32_t> && ...), "all arg should uint32_t");

    std::array<std::uint32_t, sizeof...(Ts)> values = {nums...};
    std::size_t index = 0;

    for (std::size_t i = 0; i < 3 && index < values.size(); ++i, ++index)
      vertex_idxs[i] = values[index];

    if (index >= 3) SetVertexIdxsValid();

    for (std::size_t i = 0; i < 3 && index < values.size(); ++i, ++index)
      texture_idxs[i] = values[index];

    if (index >= 6) SetTextureIdxsValid();

    for (std::size_t i = 0; i < 3 && index < values.size(); ++i, ++index)
      normal_idxs[i] = values[index];

    if (index >= 9) SetNormalIdxsValid();
  }

  void SetVertexIdxsValid() { valid_idxs |= 1; }
  void SetTextureIdxsValid() { valid_idxs |= 1 << 1; }
  void SetNormalIdxsValid() { valid_idxs |= 1 << 2; }
  bool IsVertexIdxsValid() const { return valid_idxs & 1; }
  bool IsTextureIdxsValid() const { return valid_idxs & (1 << 1); }
  bool IsNormalIdxsValid() const { return valid_idxs & (1 << 2); }
};

struct CpuSubGeometry {
  std::string material;
  std::vector<Face> faces;
};

struct CpuGeometry {
  std::string object_id;
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> textures;
  std::vector<glm::vec3> normals;
  std::vector<CpuSubGeometry> geometries;
  bool has_normals = false;
  bool has_texcoords = false;
};

}
}  // namespace AssetUtils

