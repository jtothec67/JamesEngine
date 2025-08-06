#pragma once

#include <GL/glew.h>

#include <vector>
#include <string>

namespace Renderer
{

	class Texture
	{
	public:
		Texture(const std::string& _path);
		Texture(const std::vector<std::string>& _facePaths);
		Texture(const uint8_t* _imageData, int _width, int _height, int _channels);
		~Texture() { Unload(); }
		GLuint id() const;
		void Unload();

		void GetSize(int& _width, int& _height) { _width = m_width; _height = m_height; }
		bool IsSkybox() { return skybox; }
		std::string GetPath() { return m_path; }

	private:
		mutable GLuint m_id = 0;

		int m_width = 0;
		int m_height = 0;

		mutable bool m_dirty = true;

		bool skybox = false;
		bool m_isGLTFTexture = false;
		int m_channels = 4;

		std::string m_path = "";

		std::vector<unsigned char> m_data;
		std::vector<std::string> m_syboxFaces;
	};

}