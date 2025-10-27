#pragma once

#include <GL/glew.h>

namespace Renderer
{

	enum class RenderTextureType
	{
		ColourAndDepth,
		Depth,
		BRDF_LUT
	};

	class RenderTexture
	{
	public:
		RenderTexture() {}
		RenderTexture(int _width, int _height, RenderTextureType _type = RenderTextureType::ColourAndDepth);
		~RenderTexture();

		void bind();
		void unbind();
		GLuint getTextureId();

		int getWidth() { return m_width; }
		int getHeight() { return m_height; }

		void clear();

		GLuint getFBO() { return m_fboId; }

	private:
		GLuint m_fboId = 0;
		GLuint m_texId = 0;
		GLuint m_rboId = 0;

		int m_width;
		int m_height;

		RenderTextureType m_type = RenderTextureType::ColourAndDepth;
	};
}