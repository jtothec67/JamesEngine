#pragma once

#include <GL/glew.h>

namespace Renderer
{

	enum class RenderTextureType
	{
		ColourAndDepth,
		Depth,
	};

	class RenderTexture
	{
	public:
		RenderTexture(int _width, int _height, RenderTextureType _type = RenderTextureType::ColourAndDepth);
		~RenderTexture();

		void bind();
		void unbind();
		GLuint getTextureId();

		int getWidth() { return m_width; }
		int getHeight() { return m_height; }

		void clear();

	private:
		GLuint m_fboId = 0;
		GLuint m_texId = 0;
		GLuint m_rboId = 0;

		int m_width;
		int m_height;
	};
}