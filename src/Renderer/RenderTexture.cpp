#include "RenderTexture.h"

#include <iostream>
#include <exception>

namespace Renderer
{
    void RenderTexture::destroyGL()
    {
        if (m_rboId) { glDeleteRenderbuffers(1, &m_rboId); m_rboId = 0; }
		if (m_depthTexId) { glDeleteTextures(1, &m_depthTexId); m_depthTexId = 0; }
        if (m_texId) { glDeleteTextures(1, &m_texId); m_texId = 0; }
        if (m_fboId) { glDeleteFramebuffers(1, &m_fboId);  m_fboId = 0; }
    }

    void RenderTexture::allocate()
    {
        if (m_width <= 0 || m_height <= 0)
        {
            std::cout << "RenderTexture allocate(): invalid size " << m_width << "x" << m_height << "\n";
            throw std::exception();
        }

        glGenFramebuffers(1, &m_fboId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
        if (!m_fboId)
        {
            std::cout << "Failed to generate render textures frame buffer id." << std::endl;
            throw std::exception();
        }

        glGenTextures(1, &m_texId);

        if (m_type == RenderTextureType::ColourAndDepth)
        {
            glBindTexture(GL_TEXTURE_2D, m_texId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texId, 0);

            glGenTextures(1, &m_depthTexId);
            glBindTexture(GL_TEXTURE_2D, m_depthTexId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexId, 0);

            GLenum buf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf);
        }
        else if (m_type == RenderTextureType::Colour)
        {
            glBindTexture(GL_TEXTURE_2D, m_texId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_HALF_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texId, 0);

            GLenum buf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf);
        }
        else if (m_type == RenderTextureType::Depth)
        {
            glBindTexture(GL_TEXTURE_2D, m_texId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texId, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }
        else if (m_type == RenderTextureType::IrradianceCubeMap)
        {
            // Color-only cubemap target (RGB16F), no mips
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_texId);
            for (int face = 0; face < 6; ++face)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

            // Attach one face so FBO is complete
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, m_texId, 0);
            GLenum buf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf);
        }
        else if (m_type == RenderTextureType::PrefilteredEnvCubeMap)
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_texId);

            int numMips = 1 + (int)std::floor(std::log2(std::max(m_width, m_height)));
            for (int level = 0; level < numMips; ++level)
            {
                int w = std::max(1, m_width >> level);
                int h = std::max(1, m_height >> level);
                for (int face = 0; face < 6; ++face)
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
                }
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, numMips - 1);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, m_texId, 0);
            GLenum buf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf);
        }
        else if (m_type == RenderTextureType::BRDF_LUT)
        {
            glBindTexture(GL_TEXTURE_2D, m_texId);

            // Allocate RG16F for BRDF LUT (two channels, high precision)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, m_width, m_height, 0, GL_RG, GL_HALF_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texId, 0);
            GLenum drawBuf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &drawBuf);
        }
        else if (m_type == RenderTextureType::PostProcessTarget)
        {
            glBindTexture(GL_TEXTURE_2D, m_texId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_width, m_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, m_width, m_height, 0, GL_RED, GL_HALF_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texId, 0);

            GLenum buf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf);
        }

        // Verify FBO completeness
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "RenderTexture FBO incomplete: 0x" << std::hex << status << std::dec << std::endl;
            throw std::exception();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    RenderTexture::RenderTexture(int _width, int _height, RenderTextureType _type)
        : m_fboId(0)
        , m_texId(0)
        , m_rboId(0)
		, m_depthTexId(0)
        , m_type(_type)
        , m_width(_width)
        , m_height(_height)
    {
        allocate();
    }

    RenderTexture::~RenderTexture()
    {
        destroyGL();
    }

    bool RenderTexture::resize(int newWidth, int newHeight)
    {
        if (newWidth <= 0 || newHeight <= 0)
            return false;

        if (newWidth == m_width && newHeight == m_height)
            return false;

        // Destroy and reallocate with same type and new size
        destroyGL();
        m_width = newWidth;
        m_height = newHeight;
        allocate();
        return true;
    }

    void RenderTexture::bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
    }

    void RenderTexture::unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderTexture::bindFace(int face, int level)
    {
        if (m_type != RenderTextureType::IrradianceCubeMap && m_type != RenderTextureType::PrefilteredEnvCubeMap)
        {
            std::cout << "bindFace() called on non-cubemap RenderTexture\n";
            return;
        }
        if (face < 0 || face > 5)
        {
            std::cout << "bindFace() invalid face index\n";
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

        // Make sure draw buffer is enabled for color
        GLenum buf = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &buf);

        // Attach the requested face+level as the current color target
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_texId, level);

        // Set viewport here
        int w = std::max(1, m_width >> level);
        int h = std::max(1, m_height >> level);
        glViewport(0, 0, w, h);
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

        if (m_type == RenderTextureType::IrradianceCubeMap)
        {
            // No mips: clear all 6 faces
            GLenum buf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf);

            for (int face = 0; face < 6; ++face)
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_texId, 0);
                glViewport(0, 0, m_width, m_height);
                glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }
        else if (m_type == RenderTextureType::PrefilteredEnvCubeMap)
        {
            // Clear all faces *and* mips
            GLenum buf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf);

            int numMips = 1 + (int)std::floor(std::log2(std::max(m_width, m_height)));
            for (int level = 0; level < numMips; ++level)
            {
                int w = std::max(1, m_width >> level);
                int h = std::max(1, m_height >> level);

                for (int face = 0; face < 6; ++face)
                {
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_texId, level);
                    glViewport(0, 0, w, h);
                    glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                }
            }
        }
        else if (m_type == RenderTextureType::Depth)
        {
            glClearDepth(1.0);
            glClear(GL_DEPTH_BUFFER_BIT);
        }
        else // ColourAndDepth, BRDF_LUT, etc.
        {
            GLenum buf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf);
            glViewport(0, 0, m_width, m_height);
            glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        unbind();
    }

}