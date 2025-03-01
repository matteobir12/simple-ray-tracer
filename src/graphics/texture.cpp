#include "graphics/texture.h"

namespace Graphics {

Texture::Texture()
	: m_width(0)
	, m_height(0)
	, m_image(nullptr)
	, m_textureHandle(0)
{}

Texture::~Texture()
{
	if (m_image)
	{
		delete[] m_image;
	}
}

void Texture::Init(const std::vector<Color8>& textureData, uint width, uint height)
{
	if (width <= 0 || height <= 0)
		return;

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
}

GLuint Texture::getTextureHandle()
{
	glGenTextures(1, &m_textureHandle);
	glBindTexture(GL_TEXTURE_2D, m_textureHandle);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0,
		GL_BGR_EXT, GL_UNSIGNED_BYTE, m_image);

	return m_textureHandle;
}

const byte* Texture::getImageData()
{
	return m_image;
}

}