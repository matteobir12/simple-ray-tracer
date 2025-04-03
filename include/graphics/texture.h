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
    virtual ~Texture();
    virtual void Init(const std::vector<Color8>& textureData, uint width, uint height);
    virtual GLuint getTextureHandle(GLenum binding, GLuint bindIndex, bool isImage = false);
    const byte* getImageData();
    virtual void updateImageDataFromGPU();
	void debugReadTexture();

    void setWidth(uint width) { m_width = width; }
    void setHeight(uint height) { m_height = height; }

protected:
    uint m_width;
    uint m_height;
    GLuint m_textureHandle; 
    uint m_numChannels = 4; // We're injecting an alpha channel
    uint m_dataSize;

private:
    byte* m_image;
};

class Texture32 : public Texture {
public:
    Texture32();
    virtual ~Texture32() = default;
    void InitBlank();
    virtual GLuint getTextureHandle(GLenum binding, GLuint bindIndex, bool isImage = false) override;
    virtual void updateImageDataFromGPU() override;
    float* getImageData();

private:
    float* m_image;
};
}