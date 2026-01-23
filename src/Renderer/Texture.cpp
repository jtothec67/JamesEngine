#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// Defining here as can only be defined once, we use tinygltf in Model.h
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h" // Includes stb_image and stb_image_write

#include <exception>
#include <iostream>

namespace Renderer
{

	Texture::Texture(const std::string& _path)
	{
		unsigned char* data = stbi_load(_path.c_str(), &m_width, &m_height, NULL, 4);

		if (!data)
		{
			std::cout << "Failed to load texture data: " << _path << std::endl;
			throw std::exception();
		}

		m_data.reserve(m_width * m_height * 4);
		for (size_t i = 0; i < m_width * m_height * 4; ++i)
		{
			m_data.push_back(data[i]);
		}

		stbi_image_free(data);

		skybox = false;
		m_path = _path;
	}

	// Overloaded constructor for a cubemap, used for a skybox (should be 6 faces/images)
	Texture::Texture(const std::vector<std::string>& _facePaths)
	{
		if (_facePaths.size() != 6)
		{
			std::cout << "Skybox needs 6 image paths: " << _facePaths.at(0) << std::endl;
			throw std::exception();
		}

		m_syboxFaces = _facePaths;
		skybox = true;
	}

	Texture::Texture(const uint8_t* _imageData, int _width, int _height, int _channels)
	{
		m_width = _width;
		m_height = _height;
		m_channels = _channels;
		m_isGLTFTexture = true;

		// Copy raw data into m_data
		m_data.reserve(_width * _height * _channels);
		for (int i = 0; i < _width * _height * _channels; ++i)
		{
			m_data.push_back(_imageData[i]);
		}

		skybox = false;
		m_dirty = true;
	}

	GLuint Texture::id() const
	{
		if (m_dirty)
		{
			if (!skybox)
			{
				glGenTextures(1, &m_id);

				if (!m_id)
				{
					std::cout << "Failed to load texture id" << std::endl;
					throw std::exception();
				}

				glBindTexture(GL_TEXTURE_2D, m_id);

				GLenum format = GL_RGBA;
				if (m_channels == 3) format = GL_RGB;
				else if (m_channels == 1) format = GL_RED;

				glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, &m_data.at(0));

				glGenerateMipmap(GL_TEXTURE_2D);

				glBindTexture(GL_TEXTURE_2D, 0);

				m_dirty = false;
			}
			else if (skybox)
			{
				glGenTextures(1, &m_id);

				if (!m_id)
				{
					throw std::exception();
				}

				glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

				int width, height, channels;
				for (unsigned int i = 0; i < m_syboxFaces.size(); i++)
				{
					float* data = stbi_loadf(m_syboxFaces[i].c_str(), &width, &height, &channels, 0);

					if (!data)
					{
						std::cout << "Failed to load skybox faces starting at: " << m_syboxFaces[i] << std::endl;

						stbi_image_free(data);
						throw std::exception();
					}

					GLenum srcFormat = GL_RGB;
					GLenum internalFormat = GL_RGB16F;

					if (channels == 4)
					{
						srcFormat = GL_RGBA;
						internalFormat = GL_RGBA16F;
					}
					else if (channels == 3)
					{
						srcFormat = GL_RGB;
						internalFormat = GL_RGB16F;
					}
					else
					{
						std::cout << "Unexpected HDR channel count (" << channels << ") in: " << m_syboxFaces[i] << std::endl;

						stbi_image_free(data);
						throw std::exception();
					}

					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, srcFormat, GL_FLOAT, data);

					stbi_image_free(data);
				}

				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

				m_dirty = false;
			}
		}

		return m_id;
	}

	void Texture::Unload()
	{
		if (m_id)
		{
			glDeleteTextures(1, &m_id);
			m_id = 0;
			m_dirty = true;
		}
		m_data.clear();
		m_syboxFaces.clear();
	}

}