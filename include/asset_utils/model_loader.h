#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <memory>

#include "asset_utils/types.h"

namespace AssetUtils {
std::unique_ptr<Model> LoadObject(const std::string& obj_location);

namespace Detail {
std::unique_ptr<CpuGeometry> ParseOBJ(const std::string& file_path, std::vector<std::string>* const mtl_files);

void ParseMTL(const std::string& folder_path, const std::string& file_name, std::unordered_map<std::string, Material>* libs);

std::unique_ptr<Model> ConvertCPUGeometryToModel(std::unique_ptr<CpuGeometry> cpu_geo, std::unordered_map<std::string, Material> materials);
} // namespace Detail
}  // namespace AssetUtils
