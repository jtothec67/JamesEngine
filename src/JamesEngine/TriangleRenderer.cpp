#include "TriangleRenderer.h"

namespace JamesEngine
{

	TriangleRenderer::TriangleRenderer()
	{
		Renderer::Face face;
		face.a.m_position = glm::vec3(0.0f, 1.0f, 0.0f);
		face.b.m_position = glm::vec3(-1.0f, 0.0f, 0.0f);
		face.c.m_position = glm::vec3(1.0f, 0.0f, 0.0f);
		face.a.m_texcoords = glm::vec2(0.5f, -1.f);
		face.b.m_texcoords = glm::vec2(0.f, 0.f);
		face.c.m_texcoords = glm::vec2(1.f, 0.f);

		mMesh->add(face);
	}

	void TriangleRenderer::OnRender()
	{
		glm::mat4 projection = glm::perspective(glm::radians(45.f), (float)800 / (float)600, 0.1f, 100.f);
		mShader->uniform("u_Projection", projection);

		glm::mat4 view(1.0f);
		view = glm::translate(view,glm::vec3(0.f, 0.f, 0.f));
		view = glm::rotate(view, glm::radians(0.f), glm::vec3(0, 1, 0));
		view = glm::rotate(view, glm::radians(0.f), glm::vec3(1, 0, 0));
		view = glm::rotate(view, glm::radians(0.f), glm::vec3(0, 0, 1));
		view = glm::inverse(view);
		mShader->uniform("u_View", view);

		glm::mat4 modelMatrix = glm::mat4(1.f);
		modelMatrix = glm::translate(modelMatrix, GetEntity());
		modelMatrix = glm::rotate(modelMatrix, glm::radians(0.f), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(0.f), glm::vec3(0, 1, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(0.f), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(1.f, 1.f, 1.f));
		mShader->uniform("u_Model", modelMatrix);

		mShader->uniform("u_IsSpecular", false);

		mShader->uniform("u_LightPos", glm::vec3(0.f, 0.f, 0.f));

		mShader->uniform("u_Ambient", glm::vec3(1.f, 1.f, 1.f));

		mShader->uniform("u_LightStrength", 1.f);

		mShader->draw(mMesh.get(), mTexture.get());
	}

}