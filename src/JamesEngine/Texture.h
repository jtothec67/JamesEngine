#pragma once

#include <GL/glew.h>

#include <vector>
#include <string>

class Texture
{
public:
	Texture(const std::string& _path);
	Texture(const std::vector<std::string>& _facePaths);
	~Texture() { Unload(); }
	GLuint id();
	void Unload();

	void GetSize(int& _width, int& _height) { _width = m_width; _height = m_height; }
	bool IsSkybox() { return skybox; }

private:
	GLuint m_id = 0;

	int m_width = 0;
	int m_height = 0;

	bool m_dirty = true;

	bool skybox = false;

	std::vector<unsigned char> m_data;
	std::vector<std::string> m_syboxFaces;
};