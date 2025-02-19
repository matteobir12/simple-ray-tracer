#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <memory>

namespace AssetUtils {
std::unique_ptr<Model> LoadObject(const std::string& obj_location);

namespace Detail {
std::unique_ptr<CpuGeometry> ParseOBJ(const std::string& file_path, std::vector<std::string>* const mtl_files);

void ParseMTL(const std::string& folderPath, const std::string& fileName, std::unordered_map<std::string, Material*>& material_libs);

std::unique_ptr<Model> ConvertCPUGeometryToModel(const std::unique_ptr<CpuGeometry>& cpu_geo);
} // namespace Detail
}  // namespace AssetUtils
