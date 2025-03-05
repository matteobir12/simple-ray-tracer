#pragma once

#include <GLFW/glfw3.h>
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
    GLuint getTextureHandle();
    const byte* getImageData();
	void debugReadTexture();
private:
    uint m_width;
    uint m_height;
    byte* m_image;
    GLuint m_textureHandle; 
    uint m_numChannels = 3;
};
}