#include "asset_utils/model_loader.h"

#include <sstream>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "glad/glad.h"

#include "asset_utils/gpu_texture.h"

namespace AssetUtils {
namespace {
constexpr const char* TEXTURE_FOLDER = "./textures/";
constexpr const char* OBJ_FOLDER = "./objects/";
}

// models smaller than unsigned int verts
std::unique_ptr<Model> LoadObject(const std::string& name) {
  std::vector<std::string> mtl_files;
  auto geo = Detail::ParseOBJ(OBJ_FOLDER + name + "/" + name + ".obj", &mtl_files);

  std::unordered_map<std::string, Material> material_libs;
  for (const auto& file : mtl_files)
    Detail::ParseMTL(OBJ_FOLDER + name + "/", file, &material_libs);

  if (!geo)
    throw std::runtime_error("error getting geo");

  return Detail::ConvertCPUGeometryToModel(std::move(geo), std::move(material_libs));
}

namespace Detail {
std::unique_ptr<CpuGeometry> ParseOBJ(
    const std::string& file_path,
    std::vector<std::string>* const mtl_files_to_read_ptr) {
  auto& mtl_files_to_read = *mtl_files_to_read_ptr;
  auto out_geometry = std::make_unique<CpuGeometry>();
  CpuSubGeometry current_sub_geo;

  std::filesystem::path path = file_path;
  std::ifstream file(path);
  if (!file) {
    std::cerr << "Error: Cannot open file " << file_path << std::endl;
    return nullptr;
  }

  std::string line;
  while (std::getline(file, line)) {
    line.erase(0, line.find_first_not_of(" \n\r\t"));
    line.erase(line.find_last_not_of(" \n\r\t") + 1);
    if (line.empty() || line[0] == '#') continue;
    std::istringstream linestream(line);
    std::string prefix;
    linestream >> prefix;
    if (prefix == "v") {
      glm::vec3 v;
      if (linestream >> v.x >> v.y >> v.z) {
        out_geometry->vertices.push_back(v);
      } else {
        std::cerr << "Failed to read 3 floats for vertex." << std::endl;
      }
    }
    else if (prefix == "vt") {
      glm::vec2 vt;
      if (linestream >> vt.s >> vt.t) {
        out_geometry->textures.push_back(vt);
      } else {
        std::cerr << "Failed to read 2 floats for texcoord." << std::endl;
      }
    }
    else if (prefix == "vn") {
      out_geometry->has_normals = true;
      glm::vec3 vn;
      if (linestream >> vn.x >> vn.y >> vn.z) {
        out_geometry->normals.push_back(vn);
      } else {
        std::cerr << "Failed to read 3 floats for normal." << std::endl;
      }
    }
    else if (prefix == "f") {
      std::vector<std::uint32_t> vertex_idxs;
      std::vector<std::uint32_t> texture_idxs;
      std::vector<std::uint32_t> normal_idxs;
      std::string vertex;

      while (linestream >> vertex) {
        std::istringstream vertex_stream(vertex);
        std::string v, vt, vn;

        std::getline(vertex_stream, v, '/');
        std::getline(vertex_stream, vt, '/');
        std::getline(vertex_stream, vn);

        if (!v.empty()) {
          long v_idx = std::stol(v) - 1; // OBJ indices are 1-based
          vertex_idxs.push_back(static_cast<std::uint32_t>(v_idx));
        }
        if (!vt.empty()) {
          long t_idx = std::stol(vt) - 1;
          texture_idxs.push_back(static_cast<std::uint32_t>(t_idx));
        }
        if (!vn.empty()) {
          long n_idx = std::stol(vn) - 1;
          normal_idxs.push_back(static_cast<std::uint32_t>(n_idx));
        }
      }

      if (vertex_idxs.size() != 3 && vertex_idxs.size() != 4) {
        std::cerr << "Unexpected face vertex count: " << vertex_idxs.size() << std::endl;
        continue;
      }

      Face f1({vertex_idxs[0], vertex_idxs[1], vertex_idxs[2]});
      f1.SetVertexIdxsValid();

      if (normal_idxs.size() > 2) {
        f1.normal_idxs = {normal_idxs[0], normal_idxs[1], normal_idxs[2]};
        f1.SetNormalIdxsValid();
      }
      if (texture_idxs.size() > 2) {
        f1.texture_idxs = {texture_idxs[0], texture_idxs[1], texture_idxs[2]};
        f1.SetTextureIdxsValid();
      }

      current_sub_geo.faces.push_back(f1);

      if (vertex_idxs.size() == 4) {
        Face f2({vertex_idxs[0], vertex_idxs[2], vertex_idxs[3]});
        f2.SetVertexIdxsValid();

        if (normal_idxs.size() == 4) {
          f2.normal_idxs = {normal_idxs[0], normal_idxs[2], normal_idxs[3]};
          f2.SetNormalIdxsValid();
        }
        if (texture_idxs.size() == 4) {
          f2.texture_idxs = {texture_idxs[0], texture_idxs[2], texture_idxs[3]};
          f2.SetTextureIdxsValid();
        }
        current_sub_geo.faces.push_back(f2);
      }
    }
    else if (prefix == "usemtl") {
      if (!current_sub_geo.material.empty()) {
        out_geometry->geometries.emplace_back(std::move(current_sub_geo));
        current_sub_geo = CpuSubGeometry();
      }

      std::string mtl_name;
      linestream >> mtl_name;
      current_sub_geo.material = mtl_name;
    }
    else if (prefix == "mtllib") {
      std::string mtl_file_name;
      linestream >> mtl_file_name;
      mtl_files_to_read.push_back(mtl_file_name);
    }
    else if (prefix == "s") {
      // smoothing, often ignored
    }
    else if (prefix == "o") {
      // new object, can also be ignored or used to separate geometry
    }
    else {
      std::cout << "unhandled obj line prefix: " << prefix << std::endl;
    }
  }

  if (!current_sub_geo.material.empty())
    out_geometry->geometries.push_back(std::move(current_sub_geo));

  return out_geometry;
}

void ParseMTL(
    const std::string& folder_path,
    const std::string& file_name,
    std::unordered_map<std::string, Material>* const material_libs_ptr) {
  auto& material_libs = *material_libs_ptr;
  std::filesystem::path path = folder_path + file_name;
  std::ifstream file(path);
  if (!file) {
    std::cerr << "Error: Cannot open file " << folder_path + file_name << std::endl;
    return;
  }

  Material* current_material = nullptr;
  std::string line;
  while (std::getline(file, line)) {
    line.erase(0, line.find_first_not_of(" \n\r\t"));
    line.erase(line.find_last_not_of(" \n\r\t") + 1);
    if (line.empty() || line[0] == '#') continue;
    std::istringstream linestream(line);
    std::string prefix;
    linestream >> prefix;
    bool skip_mtl = false;

    if (prefix == "newmtl") {
      skip_mtl = false;

      std::string mtl_name;
      linestream >> mtl_name;
      if (material_libs.count(mtl_name) != 0) {
        skip_mtl = true;
        std::cout << "multiple materials with the same name, skipping" << std::endl;
      } else {
        const auto res = material_libs.emplace(mtl_name, Material{});
        current_material = &((*res.first).second);
      }

      continue;
    }
    
    if (skip_mtl)
      continue;

    if (!current_material) {
      std::cerr << "Material not defined" << std::endl;
      continue;
    }

    if (prefix == "map_Kd") {
      std::string texture_name;
      linestream >> texture_name;
      current_material->use_texture = true;

      // Use 'file_name' rather than 'fileName'
      std::string tex_path =
          TEXTURE_FOLDER +
          file_name.substr(0, file_name.size() - 4) + // remove .mtl extension
          "/" + texture_name;
      current_material->texture = GPUTexture(tex_path);
    }
    else if (prefix == "Kd") {
      glm::vec3 d;
      linestream >> d.x >> d.y >> d.z;
      current_material->diffuse = d;
    }
    else if (prefix == "Ka") {
      // Ambient reflectivity (ignored or stored if needed)
    }
    else if (prefix == "Tf") {
      // Transmission filter (ignored or stored if needed)
    }
    else if (prefix == "Ni") {
      // Index of refraction (ignored or stored if needed)
    }
    else if (prefix == "Ks") {
      glm::vec3 s;
      linestream >> s.x >> s.y >> s.z;
      current_material->specular = s;
    }
    else if (prefix == "Ns") {
      float e;
      linestream >> e;
      current_material->specular_ex = e;
    }
    else {
      std::cout << "Unknown material property: " << prefix << std::endl;
    }
  }
}

std::unique_ptr<Model> ConvertCPUGeometryToModel(
    std::unique_ptr<CpuGeometry> cpu_geo,
    std::unordered_map<std::string, Material> materials) {
  std::vector<Material> model_materials; 
  model_materials.reserve(materials.size());
  
  std::unordered_map<std::string, std::uint32_t> material_index_map;
  material_index_map.reserve(materials.size());

  for (const auto& kv : materials) {
    const std::string& mat_name = kv.first;
    const Material& mat_val = kv.second;
    material_index_map[mat_name] = static_cast<std::uint32_t>(model_materials.size());
    model_materials.push_back(mat_val);
  }

  std::vector<GPU::PackedVertexData> packed_verts;
  packed_verts.reserve(3 * 1000); // Calc real capacity

  std::vector<GPU::Triangle> all_triangles;
  all_triangles.reserve(1000); // Calc real capacity

  const auto push_vertex = [&packed_verts](const glm::vec3& pos, const glm::vec2& tex) -> std::uint32_t {
    packed_verts.emplace_back(pos, tex);
    return static_cast<std::uint32_t>(packed_verts.size() - 1);
  };

  for (const auto& sub_geo : cpu_geo->geometries) {
    std::uint32_t material_idx = 0;
    const auto it = material_index_map.find(sub_geo.material);
    if (it != material_index_map.cend()) {
      material_idx = it->second;
    } else {
      std::cout << "Couldn't find expected material" << std::endl;
    }

    for (const auto& face : sub_geo.faces) {
      GPU::Triangle tri;
      tri.material_idx = material_idx;
      for (int corner = 0; corner < 3; ++corner) {
        const glm::vec3& pos = cpu_geo->vertices[face.vertex_idxs[corner]];

        glm::vec2 uv(0.0f);
        if (cpu_geo->has_texcoords && face.IsTextureIdxsValid())
          uv = cpu_geo->textures[face.texture_idxs[corner]];

          tri.vertex_idxs[corner] = push_vertex(pos, uv);
      }

      all_triangles.push_back(tri);
    }
  }

  const auto center_fn = [&packed_verts](const GPU::Triangle& tri) -> glm::vec3 {
    const glm::vec3& p0 = packed_verts[tri.vertex_idxs[0]].vertex;
    const glm::vec3& p1 = packed_verts[tri.vertex_idxs[1]].vertex;
    const glm::vec3& p2 = packed_verts[tri.vertex_idxs[2]].vertex;
    return (p0 + p1 + p2) / 3.0f;
  };

  const auto bounds_fn = [&packed_verts](const GPU::Triangle& tri) -> std::pair<glm::vec3, glm::vec3> {
    const glm::vec3& p0 = packed_verts[tri.vertex_idxs[0]].vertex;
    const glm::vec3& p1 = packed_verts[tri.vertex_idxs[1]].vertex;
    const glm::vec3& p2 = packed_verts[tri.vertex_idxs[2]].vertex;

    std::pair<glm::vec3, glm::vec3> out = std::make_pair(p0, p0);
    out.first = glm::min(out.first, p1);
    out.first = glm::min(out.first, p2);
    out.second = glm::max(out.second, p1);
    out.second = glm::max(out.second, p2);

    return out;
  };

  IntersectionUtils::BVH<GPU::Triangle> bvh(
      all_triangles, 
      center_fn, 
      bounds_fn);

  auto model = std::make_unique<Model>();
  model->model_bvh = std::move(bvh);
  model->model_materials = std::move(model_materials);

  return model;
}

}
}
