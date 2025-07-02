#include "ModelRenderer.h"

#include "Model.h"
#include "Texture.h"
#include "Shader.h"
#include "Entity.h"
#include "Transform.h"
#include "Core.h"
#include "Resources.h"
#include "Camera.h"
#include "LightManager.h"

#include <glm/glm.hpp>
#include <iostream>
#include <exception>

namespace JamesEngine
{

	void ModelRenderer::OnInitialize()
	{
		mShader = GetEntity()->GetCore()->GetResources()->Load<Shader>("shaders/ObjShader");
		mDepthShader = GetEntity()->GetCore()->GetResources()->Load<Shader>("shaders/DepthOnly");
	}

	void ModelRenderer::OnRender()
	{
		if (!mModel)
			return;

		std::shared_ptr<Core> core = GetEntity()->GetCore();

		std::shared_ptr<Camera> camera = core->GetCamera();

		mShader->mShader->uniform("u_Projection", camera->GetProjectionMatrix());

		mShader->mShader->uniform("u_View", camera->GetViewMatrix());

		glm::mat4 entityModel = GetEntity()->GetComponent<Transform>()->GetModel();

		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(mRotationOffset.x), glm::vec3(1, 0, 0)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(mRotationOffset.y), glm::vec3(0, 1, 0)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(mRotationOffset.z), glm::vec3(0, 0, 1));
		glm::mat4 offsetMatrix = glm::translate(glm::mat4(1.0f), mPositionOffset) * rotationMatrix;

		glm::mat4 model = entityModel * offsetMatrix;

		mShader->mShader->uniform("u_Model", model);

		std::vector<std::shared_ptr<Light>> lights = core->GetLightManager()->GetLights();

		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> colors;
		std::vector<float> strengths;

		for (const auto& light : lights)
		{
			positions.push_back(light->position);
			colors.push_back(light->colour);
			strengths.push_back(light->strength);
		}

		mShader->mShader->uniform("u_LightPositions", positions);
		mShader->mShader->uniform("u_LightColors", colors);
		mShader->mShader->uniform("u_LightStrengths", strengths);

		mShader->mShader->uniform("u_Ambient", core->GetLightManager()->GetAmbient());

		mShader->mShader->uniform("u_SpecStrength", mSpecularStrength);

		mShader->mShader->uniform("u_DirLightDirection", core->GetLightManager()->GetDirectionalLightDirection());
		mShader->mShader->uniform("u_DirLightColor", core->GetLightManager()->GetDirectionalLightColour());

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, core->GetLightManager()->GetDirectionalLightShadowMap()->getTextureId());
		//glBindTexture(GL_TEXTURE_2D, core->GetResources()->Load<Texture>("images/mouse")->mTexture->id());

		mShader->mShader->uniform("u_ShadowMap", 3);
		mShader->mShader->uniform("u_LightSpaceMatrix", core->GetLightSpaceMatrix());

		std::vector<Renderer::Texture*> rawTextures;
		for (const auto& tex : mTextures)
		{
			rawTextures.push_back(tex->mTexture.get());
		}

		// Call the updated draw function with support for multi-materials
		mShader->mShader->draw(mModel->mModel.get(), rawTextures);
	}

	void ModelRenderer::OnShadowRender()
	{
		if (!mModel)
			return;

		glm::mat4 entityModel = GetEntity()->GetComponent<Transform>()->GetModel();

		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(mRotationOffset.x), glm::vec3(1, 0, 0)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(mRotationOffset.y), glm::vec3(0, 1, 0)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(mRotationOffset.z), glm::vec3(0, 0, 1));
		glm::mat4 offsetMatrix = glm::translate(glm::mat4(1.0f), mPositionOffset) * rotationMatrix;

		glm::mat4 model = entityModel * offsetMatrix;

		mDepthShader->mShader->uniform("u_Model", model);

		mDepthShader->mShader->uniform("u_LightSpaceMatrix", GetEntity()->GetCore()->GetLightSpaceMatrix());
		mDepthShader->mShader->uniform("u_AlphaCutoff", 0.5f);
		mDepthShader->mShader->uniform("u_UseAlphaCutoff", 1);


		std::vector<Renderer::Texture*> rawTextures;
		for (const auto& tex : mTextures)
		{
			rawTextures.push_back(tex->mTexture.get());
		}

		// Call the updated draw function with support for multi-materials
		mDepthShader->mShader->draw(mModel->mModel.get(), rawTextures);
	}

}