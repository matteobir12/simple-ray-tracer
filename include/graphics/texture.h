#pragma once

//#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <vector>

#include "common/types.h"
#include "raytracer/raytracer_types.h"

namespace Graphics {

using byte = unsigned char;
using uint = Common::uint;
using Color8 = RayTracer::Color8;

class Texture {
public:
    Texture();
    ~Texture();
    void Init(const std::vector<Color8>& textureData, uint width, uint height);
    GLuint getTextureHandle(bool isImage = false);
    const byte* getImageData();
	void debugReadTexture();

    void setWidth(uint width) { m_width = width; }
    void setHeight(uint height) { m_height = height; }

private:
    uint m_width;
    uint m_height;
    byte* m_image;
    GLuint m_textureHandle; 
    uint m_numChannels = 4; // We're injecting an alpha channel
};
}