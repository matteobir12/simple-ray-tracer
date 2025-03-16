#include "asset_utils/gpu_loader.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <iostream>

#include "intersection_utils/bvh.h"

namespace AssetUtils {
namespace {
struct GPUBVH {
  std::uint32_t first_index;
  std::uint32_t count;
  std::uint32_t _pad0;
  std::uint32_t _pad1;
  glm::mat4 frame = glm::mat4(1);
};

struct GPUMaterial {
  glm::vec3 diffuse;
  glm::vec3 specular;
  float specular_ex;
  std::uint32_t use_texture; // 0 or 1
};

struct GPUTriangle {
  std::uint32_t v0_idx;
  std::uint32_t v1_idx;
  std::uint32_t v2_idx;
  std::uint32_t material_idx;
};

// each element = 1 BVH per model
static std::vector<GPUBVH> g_bvhs;       
// all BVH nodes from all models
static std::vector<IntersectionUtils::BVHNode> g_bvh_nodes;
// all materials from all models
static std::vector<GPUMaterial> g_materials;
// all triangles from all models
static std::vector<GPUTriangle> g_triangles;
// all vertices from all models
static std::vector<GPU::PackedVertexData> g_vertices;

static GLuint s_bvh_ranges_SSBO = 0;
static GLuint s_bvh_nodes_SSBO = 0;
static GLuint s_materials_SSBO = 0;
static GLuint s_triangles_SSBO = 0;
static GLuint s_vertices_SSBO = 0;

static GLuint s_ray_buffer = 0;
}

void UploadModelDataToGPU(const std::vector<Model*>& models) {
  g_bvhs.clear();
  g_bvh_nodes.clear();
  g_materials.clear();
  g_triangles.clear();
  g_vertices.clear();

  std::uint32_t cur_BVH_node_off = 0;
  std::uint32_t cur_triangle_off = 0;
  std::uint32_t cur_material_off = 0;
  std::uint32_t cur_vertex_off = 0;

  for (const auto& model_ptr : models) {
    if (!model_ptr)
      throw std::runtime_error("Model was null!");
    const Model& model = *model_ptr;

    std::uint32_t model_mat_off = cur_material_off;
    for (const auto& mat : model.model_materials) {
      GPUMaterial gpu_mat;
      gpu_mat.diffuse = mat.diffuse;
      gpu_mat.specular = mat.specular;
      gpu_mat.specular_ex = mat.specular_ex;
      gpu_mat.use_texture = mat.use_texture;

      g_materials.push_back(gpu_mat);
    }

    cur_material_off += static_cast<std::uint32_t>(model.model_materials.size());

    std::uint32_t model_vert_off = cur_vertex_off;
    for (const auto& v : model.vertex_data_buffer)
      g_vertices.push_back(v);

    cur_vertex_off += static_cast<std::uint32_t>(model.vertex_data_buffer.size());

    const auto& bvh = model.model_bvh;
    GPUBVH gpu_BVH;
    gpu_BVH.first_index = cur_BVH_node_off;
    gpu_BVH.count = static_cast<std::uint32_t>(bvh.GetBVH().size());
    g_bvhs.push_back(gpu_BVH);

    std::uint32_t localTriOffset = cur_triangle_off;
    for (const auto& tri : bvh.GetPrims()) {
      GPUTriangle gpu_tri;
      gpu_tri.v0_idx = tri.vertex_idxs[0] + model_vert_off;
      gpu_tri.v1_idx = tri.vertex_idxs[1] + model_vert_off;
      gpu_tri.v2_idx = tri.vertex_idxs[2] + model_vert_off;
      gpu_tri.material_idx = tri.material_idx + model_mat_off;
      g_triangles.push_back(gpu_tri);
    }

    std::uint32_t num_BVH_tri = static_cast<std::uint32_t>(bvh.GetPrims().size());
    cur_triangle_off += num_BVH_tri;

    for (const auto& node : bvh.GetBVH()) {
      IntersectionUtils::BVHNode gpu_node;
      gpu_node.min_bounds = node.min_bounds;
      gpu_node.max_bounds = node.max_bounds;
      gpu_node.first_child = node.first_child + cur_BVH_node_off;
      gpu_node.first_prim_index = node.first_prim_index + localTriOffset;
      gpu_node.prim_count = node.prim_count;
      g_bvh_nodes.push_back(gpu_node);
    }

    cur_BVH_node_off += gpu_BVH.count;
  }

  for (auto* const buff_id : {&s_bvh_ranges_SSBO, &s_bvh_nodes_SSBO, &s_materials_SSBO, &s_triangles_SSBO, &s_vertices_SSBO}) {
    if (*buff_id == 0)
      glGenBuffers(1, buff_id);
    else
      std::cout << "Not recommended to re-call this function. Currently buffers are created with GL_STATIC_DRAW" << std::endl;
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_bvh_ranges_SSBO);
  glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      g_bvhs.size() * sizeof(GPUBVH),
      g_bvhs.data(),
      GL_DYNAMIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_bvh_nodes_SSBO);
  glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      g_bvh_nodes.size() * sizeof(IntersectionUtils::BVHNode),
      g_bvh_nodes.data(),
      GL_STATIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_materials_SSBO);
  glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      g_materials.size() * sizeof(GPUMaterial),
      g_materials.data(),
      GL_STATIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_triangles_SSBO);
  glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      g_triangles.size() * sizeof(GPUTriangle),
      g_triangles.data(),
      GL_STATIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_vertices_SSBO);
  glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      g_vertices.size() * sizeof(GPU::PackedVertexData),
      g_vertices.data(),
      GL_STATIC_DRAW);

  // bind them to the binding points that match the shader
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_bvh_ranges_SSBO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_bvh_nodes_SSBO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_materials_SSBO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s_triangles_SSBO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, s_vertices_SSBO);
}

void UpdateModelMatrix(const std::uint32_t index, const glm::mat4& matrix) {
  g_bvhs.at(index).frame = matrix;

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_bvh_ranges_SSBO);
  const std::size_t offset = index * sizeof(GPUBVH);
  const std::size_t matrix_offset = offset + offsetof(GPUBVH, frame);
  glBufferSubData(
      GL_SHADER_STORAGE_BUFFER,
      matrix_offset,
      sizeof(glm::mat4),
      &g_bvhs.at(index).frame);
}

void UpdateRays(const std::uint32_t ray_count, const Common::Ray* const ray_buffer) {
  if (s_ray_buffer == 0)
    glGenBuffers(1, &s_ray_buffer);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_ray_buffer);
  // GL_DYNAMIC_COPY if the gpu writes to it, otherwise GL_DYNAMIC_DRAW
  glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      ray_count * sizeof(Common::Ray),
      ray_buffer,
      GL_DYNAMIC_COPY);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, s_ray_buffer);
}
}
