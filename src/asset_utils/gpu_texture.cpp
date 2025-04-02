#include "asset_utils/gpu_texture.h"

namespace AssetUtils {
  std::unordered_map<std::string, AssetUtils::Detail::TextureInfo> AssetUtils::GPUTexture::LoadedTextures;
}