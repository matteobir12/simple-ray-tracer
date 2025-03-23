#pragma once

#include "glad/glad.h"

#include <vector>
#include <memory>

#include "asset_utils/types.h"
#include "common/types.h"

namespace AssetUtils {

// Due to how models are currently loaded onto the gpu in one large contiguous buffer
// this function should be called with all models expected to be in scene.
// There is no real streaming support other than simply recalling this function to reload
// gpu data.
//
// For now compute shader program should be bound before calling. (might change)
void UploadModelDataToGPU(const std::vector<Model*>& models, const std::uint32_t binding_offset = 0);

// Index should be the same as the index in the vector passed to UploadModelDataToGPU
//
// Might want to create either a multiupload version or a flush to prevent multiple buffer uploads
//
// For now compute shader program should be bound before calling. (might change)
void UpdateModelMatrix(const std::uint32_t index, const glm::mat4& matrix);

void UpdateRays(const std::uint32_t ray_count, const Common::Ray* const ray_buffer);
}
