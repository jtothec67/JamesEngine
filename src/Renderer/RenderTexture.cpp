#include "RenderTexture.h"

#include <iostream>
#include <exception>

namespace Renderer
{
	RenderTexture::RenderTexture(int _width, int _height, RenderTextureType _type)
		: m_fboId(0)
		, m_texId(0)
		, m_rboId(0)
		, m_type(_type)
	{
		m_width = _width;
		m_height = _height;

		glGenFramebuffers(1, &m_fboId);
		if (!m_fboId)
		{
			std::cout << "Failed to generate render textures frame buffer id." << std::endl;
			throw std::exception();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

		glGenTextures(1, &m_texId);
		glBindTexture(GL_TEXTURE_2D, m_texId);

		if (_type == RenderTextureType::Depth)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0,
				GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texId, 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texId, 0);

			glGenRenderbuffers(1, &m_rboId);
			glBindRenderbuffer(GL_RENDERBUFFER, m_rboId);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rboId);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	RenderTexture::~RenderTexture()
	{
		glDeleteFramebuffers(1, &m_fboId);
		glDeleteTextures(1, &m_texId);
		glDeleteRenderbuffers(1, &m_rboId);
	}

	void RenderTexture::bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
	}

	void RenderTexture::unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	GLuint RenderTexture::getTextureId()
	{
		if (!m_texId)
		{
			std::cout << "Render Texture id has not been generated." << std::endl;
			throw std::exception();
		}

		return m_texId;
	}

	void RenderTexture::clear()
	{
		bind();

		if (m_type == RenderTextureType::Depth)
		{
			glClearDepth(1.0f);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		else
		{
			glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		unbind();
	}

}