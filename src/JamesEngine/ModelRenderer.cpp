#include "ModelRenderer.h"

#include "Model.h"
#include "Texture.h"
#include "Shader.h"
#include "Entity.h"
#include "Transform.h"
#include "Core.h"
#include "Resources.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <iostream>
#include <exception>

namespace JamesEngine
{

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

		std::shared_ptr<Core> core = GetEntity()->GetCore();

		std::shared_ptr<Camera> camera = core->GetCamera();

		mShader->mShader->uniform("u_Projection", camera->GetProjectionMatrix());

		mShader->mShader->uniform("u_View", camera->GetViewMatrix());

		mShader->mShader->uniform("u_Model", GetEntity()->GetComponent<Transform>()->GetModel());

		mShader->mShader->uniform("u_LightPos", core->GetLightPosition());

		mShader->mShader->uniform("u_LightStrength", core->GetLightStrength());

		mShader->mShader->uniform("u_LightColor", core->GetLightColor());

		mShader->mShader->uniform("u_Ambient", core->GetAmbient());

		mShader->mShader->uniform("u_SpecStrength", mSpecularStrength);

		mShader->mShader->draw(mModel->mModel.get(), mTexture->mTexture.get());
	}

}