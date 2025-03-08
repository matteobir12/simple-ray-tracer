#pragma once

#include "glad/glad.h"

#include <vector>
#include <memory>

#include "asset_utils/types.h"

namespace AssetUtils {

// Due to how models are currently loaded onto the gpu in one large contiguous buffer
// this function should be called with all models expected to be in scene.
// There is no real streaming support other than simply recalling this function to reload
// gpu data.
void UploadModelDataToGPU(const std::vector<Model*>& models);
}
