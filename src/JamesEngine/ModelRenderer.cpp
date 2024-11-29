#include "ModelRenderer.h"

#include "Model.h"
#include "Texture.h"
#include "Shader.h"
#include "Entity.h"
#include "Transform.h"
#include "Core.h"
#include "Resources.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace JamesEngine
{

	ModelRenderer::ModelRenderer()
	{

	}

	void ModelRenderer::OnInitialize()
	{
		mShader = GetEntity()->GetCore()->GetResources()->Load<Shader>("shaders/ObjShader");
	}

	void ModelRenderer::OnRender()
	{
		if (!mModel)
			return;

		if (!mTexture)
			return;

		int winWidth, winHeight;
		GetEntity()->GetCore()->GetWindow()->GetWindowSize(winWidth, winHeight);
		glm::mat4 projection = glm::perspective(glm::radians(45.f), (float)winWidth / (float)winHeight, 0.1f, 100.f);
		mShader->mShader->uniform("u_Projection", projection);

		glm::mat4 view(1.0f);
		view = glm::translate(view, glm::vec3(0.f, 0.f, 0.f));
		view = glm::rotate(view, glm::radians(0.f), glm::vec3(0, 1, 0));
		view = glm::rotate(view, glm::radians(0.f), glm::vec3(1, 0, 0));
		view = glm::rotate(view, glm::radians(0.f), glm::vec3(0, 0, 1));
		view = glm::inverse(view);
		mShader->mShader->uniform("u_View", view);

		Transform* transform = GetEntity()->GetComponent<Transform>().get();

		
		mShader->mShader->uniform("u_Model", transform->GetModel());

		mShader->mShader->uniform("u_IsSpecular", false);

		mShader->mShader->uniform("u_LightPos", glm::vec3(0.f, 0.f, 0.f));

		mShader->mShader->uniform("u_Ambient", glm::vec3(1.f, 1.f, 1.f));

		mShader->mShader->uniform("u_LightStrength", 1.f);

		mShader->mShader->draw(mModel->mModel.get(), mTexture->mTexture.get());
	}

}