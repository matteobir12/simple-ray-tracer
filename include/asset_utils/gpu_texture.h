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

}

class GPUTexture {
  static std::unordered_map<std::string, Detail::TextureInfo> LoadedTextures;
 public:
  GPUTexture() : texture_id_(0) {}
  explicit GPUTexture(const std::string& file_name, const bool make_res_arb) : valid_for_bindless_(make_res_arb) {
    const auto it = LoadedTextures.find(file_name);
    if (it != LoadedTextures.cend()) {
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

    if (make_res_arb) {
      handle_ = glGetTextureHandleARB(texture_id_);
      glMakeTextureHandleResidentARB(handle_); 
    }

    stbi_image_free(data);
    LoadedTextures[file_name] = {texture_id_, 1};
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  GPUTexture(const GPUTexture& other) 
    : texture_id_(other.texture_id_) {
    for (auto& [_, info] : LoadedTextures) {
      if (info.texture_gpu_id == texture_id_) {
        info.ref_count += 1;
        break;
      }
    }
  }

  GPUTexture(GPUTexture&& other) noexcept
    : texture_id_(other.texture_id_)
  {
    other.texture_id_ = 0;
  }

  GPUTexture& operator=(const GPUTexture& other) {
    if (this != &other) {
      Release();
      texture_id_ = other.texture_id_;
      for (auto& [_, info] : LoadedTextures) {
        if (info.texture_gpu_id == texture_id_) {
          info.ref_count += 1;
          break;
        }
      }
    }
    return *this;
  }

  GPUTexture& operator=(GPUTexture&& other) noexcept {
    if (this != &other) {
      Release();
      texture_id_ = other.texture_id_;
      other.texture_id_ = 0;
    }
    return *this;
  }

  ~GPUTexture() {
    Release();
  }

  void Release() {
    if (texture_id_ == 0) return;
    for (auto it = LoadedTextures.begin(); it != LoadedTextures.end(); ++it) {
      if (it->second.texture_gpu_id == texture_id_) {
        it->second.ref_count -= 1;
        if (it->second.ref_count <= 0) {
          glDeleteTextures(1, &texture_id_);
          LoadedTextures.erase(it);
        }
        break;
      }
    }
    texture_id_ = 0;
  }

  GLuint GetID() const { return texture_id_; }
  GLuint GetHandle() const {
    if (valid_for_bindless_)
      return handle_;
    throw std::runtime_error("not valid for bindless");
  }

 private:
  GLuint texture_id_;
  bool valid_for_bindless_ = false;
  GLuint64 handle_;
};

}  // namespace AssetUtils
