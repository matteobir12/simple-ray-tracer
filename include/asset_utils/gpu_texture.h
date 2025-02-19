#pragma once

#include <unordered_map>
#include <string>
#include <stdexcept>

#include "stb_image.h"

#include "glad/glad.h"

namespace AssetUtils {
namespace Detail {
struct TextureInfo {
  GLuint texture_gpu_id = 0;
  int ref_count = 0;
};

std::unordered_map<std::string, TextureInfo> LoadedTextures;
}

class GPUTexture {
 public:
  GPUTexture() : texture_id_(0) {}
  explicit GPUTexture(const std::string& file_name) {
    const auto it = Detail::LoadedTextures.find(file_name);
    if (it != Detail::LoadedTextures.cend()) {
      texture_id_ = it->second.texture_gpu_id;
      it->second.ref_count += 1;
    }

    int width, height, n_channels;
    unsigned char* const data = stbi_load(file_name.c_str(), &width, &height, &n_channels, 0);
    if (!data)
      throw std::runtime_error("faild to load texture file");

    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    const GLenum format = [n_channels](){
      if (n_channels == 1)
          return GL_RED;

      if (n_channels == 3)
        return GL_RGB;

      if (n_channels == 4)
        return GL_RGBA;

      return GL_RGB;
    }();

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    Detail::LoadedTextures[file_name] = {texture_id_, 1};
  }

  ~GPUTexture() {
    for (auto it = Detail::LoadedTextures.begin(); it != Detail::LoadedTextures.end(); ++it) {
      if (it->second.texture_gpu_id == texture_id_) {
        it->second.ref_count -= 1;
        if (it->second.ref_count <= 0 ) {
          Detail::LoadedTextures.erase(it);
          if (texture_id_)
            glDeleteTextures(1, &texture_id_);

          break;
        }
      }
    }
  }

  GLuint GetID() const { return texture_id_; }

 private:
  GLuint texture_id_;
};

}  // namespace AssetUtils
