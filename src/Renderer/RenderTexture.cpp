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
		glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
		if (!m_fboId)
		{
			std::cout << "Failed to generate render textures frame buffer id." << std::endl;
			throw std::exception();
		}
		
		glGenTextures(1, &m_texId);
		glBindTexture(GL_TEXTURE_2D, m_texId);

		if (_type == RenderTextureType::Depth)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0,
				GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texId, 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}
		else if (_type == RenderTextureType::ColourAndDepth)
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
		else if (_type == RenderTextureType::BRDF_LUT)
		{
			glBindTexture(GL_TEXTURE_2D, m_texId);

			// Allocate RG16F for BRDF LUT (two channels, high precision)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, m_width, m_height, 0, GL_RG, GL_HALF_FLOAT, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			// No mipmaps for BRDF LUT — only level 0
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texId, 0);
			GLenum drawBuf = GL_COLOR_ATTACHMENT0;
			glDrawBuffers(1, &drawBuf);
		}

		// 3) Verify FBO completeness (very important for depth-only targets)
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "RenderTexture FBO incomplete: 0x" << std::hex << status << std::dec << std::endl;
			throw std::exception();
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