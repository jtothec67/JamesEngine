#include "TriangleRenderer.h"

namespace JamesEngine
{

	TriangleRenderer::TriangleRenderer()
	{
		Renderer::Face face;
		face.a.m_position = glm::vec3(0.0f, 1.0f, 0.0f);
		face.b.m_position = glm::vec3(-1.0f, 0.0f, 0.0f);
		face.c.m_position = glm::vec3(1.0f, 0.0f, 0.0f);

		mMesh->add(face);
	}

	void TriangleRenderer::OnRender()
	{
		mShader->draw(mMesh.get(), mTexture.get());
	}

}