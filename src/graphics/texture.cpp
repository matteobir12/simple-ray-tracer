#include "graphics/texture.h"
#include <iostream>

namespace Graphics {

Texture::Texture()
    : m_width(0), m_height(0), m_image(nullptr), m_textureHandle(0)
{}

Texture::~Texture()
{
    if (m_image) {
        delete[] m_image;
    }
    if (m_textureHandle) {
        glDeleteTextures(1, &m_textureHandle);
    }
}

void Texture::Init(const std::vector<Color8>& textureData, uint width, uint height)
{
    if (width <= 0 || height <= 0)
        return;

    m_width = width;
    m_height = height;

    size_t imgSize = m_numChannels * textureData.size();
    m_image = new byte[imgSize];

    for (uint i = 0; i < textureData.size(); ++i)
    {
        Color8 c = textureData[i];
        uint start = i * m_numChannels;
        for (uint j = 0; j < m_numChannels; ++j)
        {
            m_image[start + j] = c[j];
        }
    }

    if (!textureData.empty()) {
        Color8 firstPixel = textureData[0];
        std::cout << "First pixel: R=" << (int)firstPixel.r << " G=" << (int)firstPixel.g << " B=" << (int)firstPixel.b << std::endl;
    }
}

GLuint Texture::getTextureHandle()
{
    if (m_textureHandle == 0) {
        std::cout << "Texture::getTextureHandle => current context: "
          << glfwGetCurrentContext() << std::endl;

        glGenTextures(1, &m_textureHandle);

        std::cout << "Texture::getTextureHandle => glGenTextures returned: " << m_textureHandle << std::endl;
        
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after glGenTextures: " << err << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, m_textureHandle);

        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after glBindTexture: " << err << std::endl;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, m_image);
		
		err = glGetError();
		if (err != GL_NO_ERROR) {
			std::cerr << "OpenGL error after glTexImage2D: " << err << std::endl;
		}

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return m_textureHandle;
}

const byte* Texture::getImageData()
{
    return m_image;
}

void Texture::debugReadTexture() {
	if(m_textureHandle == 0) {
		std::cerr << "Texture handle is 0" << std::endl;
		return;
	}
	glBindTexture(GL_TEXTURE_2D, m_textureHandle);
	byte* data = new byte[m_width * m_height * m_numChannels];
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	std::cout << "First pixel from texture: R=" << (int)data[0] << " G=" << (int)data[1] << " B=" << (int)data[2] << std::endl;
	delete[] data;
	glBindTexture(GL_TEXTURE_2D, 0);
}

}