#pragma once

#include <unordered_map>
#include <string>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glad/glad.h"

namespace AssetUtils {

class GPUTexture {
  static std::unordered_map<std::string, GLuint> LoadedTextures_;

  GPUTexture(const std::string& file_name) {
    const auto it = LoadedTextures_.find(file_name);
    if (it == LoadedTextures_.cend())
      return it->second;

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
  }

  ~GPUTexture() {
    LoadedTextures_.erase(texture_id_);

    if (texture_id_)
      glDeleteTextures(1, &texture_id_)
  }

  GLuint GetID() const { return texture_id_ }

 private:
  GLuint texture_id_;
}
}  // namespace AssetUtils
