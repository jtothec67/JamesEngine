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

#include "Renderer/RenderTexture.h"

#include <glm/glm.hpp>
#include <iostream>
#include <exception>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace JamesEngine
{

	void ModelRenderer::OnInitialize()
	{
		mShader = GetEntity()->GetCore()->GetResources()->Load<Shader>("shaders/ObjShader");
		mDepthShader = GetEntity()->GetCore()->GetResources()->Load<Shader>("shaders/DepthOnly");
	}

	void ModelRenderer::OnAlive()
	{
		if (!mPreBakeShadows || !mModel)
			return;

		// Prebake a shadow map for static models
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_MULTISAMPLE);
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

		//glm::vec2 fullOrthoSize = glm::vec2(mModel->mModel->get_width(), mModel->mModel->get_length());
		//glm::vec2 halfOrthoSize = fullOrthoSize * 0.5f;

		//float nearPlane = 1.0f;
		//float farPlane = 1000.0f;

		//glm::vec3 lightDir = glm::normalize(GetCore()->GetLightManager()->GetDirectionalLightDirection());

		//// Loop over 2x2 tiles
		//for (int y = 0; y < 1; ++y)
		//{
		//	for (int x = 0; x < 1; ++x)
		//	{
		//		// Offset to cover one tile
		//		glm::vec2 tileOffset = glm::vec2(
		//			(x - 0.5f) * halfOrthoSize.x * 2.0f,
		//			(y - 0.5f) * halfOrthoSize.y * 2.0f
		//		);

		//		// Compute scene center for this tile
		//		glm::vec3 sceneCenter = mPositionOffset + glm::vec3(tileOffset.x, -65.0f, tileOffset.y);

		//		glm::vec3 lightPos = sceneCenter - lightDir * (farPlane / 2.0f);
		//		glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0, 1, 0));

		//		glm::mat4 lightProjection = glm::ortho(
		//			-halfOrthoSize.x, halfOrthoSize.x,
		//			-halfOrthoSize.y, halfOrthoSize.y,
		//			nearPlane, farPlane
		//		);

		//		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		//		auto renderTexture = std::make_shared<Renderer::RenderTexture>(32768/2, 32768/2, Renderer::RenderTextureType::Depth);

		//		renderTexture->clear();
		//		renderTexture->bind();
		//		glViewport(0, 0, renderTexture->getWidth(), renderTexture->getHeight());

		//		mPreBakeShadows = false;
		//		OnShadowRender(lightSpaceMatrix);
		//		mPreBakeShadows = true;

		//		renderTexture->unbind();

		//		GetCore()->GetLightManager()->AddPreBakedShadowMap(renderTexture, lightSpaceMatrix);
		//	}
		//}

		glm::vec2 orthoSize;

		if (mCustomShadowMapSize.x <= 0 || mCustomShadowMapSize.y <= 0)
			orthoSize = glm::vec2(mModel->mModel->get_width() / 2, mModel->mModel->get_length() / 2);
		else
			orthoSize = glm::vec2(mCustomShadowMapSize.x / 2, mCustomShadowMapSize.y / 2);

		float nearPlane = 0.1f;
		float farPlane = 1000.f;

		glm::vec3 sceneCenter = GetPosition() + mPositionOffset + mCustomCenter;

		glm::vec3 lightDir = glm::normalize(GetCore()->GetLightManager()->GetDirectionalLightDirection());
		glm::vec3 lightPos = sceneCenter - lightDir * (farPlane / 2.f);

		glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0, 1, 0));

		glm::mat4 lightProjection = glm::ortho(
			-orthoSize.x, orthoSize.x,
			-orthoSize.y, orthoSize.y,
			nearPlane, farPlane
		);

		// 3. Compute light-space matrix
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		std::shared_ptr<Renderer::RenderTexture> renderTexture = std::make_shared<Renderer::RenderTexture>(32768, 32768, Renderer::RenderTextureType::Depth);

		renderTexture->clear();
		renderTexture->bind();
		glViewport(0, 0, renderTexture->getWidth(), renderTexture->getHeight());

		mPreBakeShadows = false; // Disable pre-baking shadows after the first bake

		OnShadowRender(lightSpaceMatrix);

		mPreBakeShadows = true; // Re-enable pre-baking shadows for future renders

		renderTexture->unbind();

		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

		GetCore()->GetLightManager()->AddPreBakedShadowMap(renderTexture, lightSpaceMatrix);
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

		std::vector<std::shared_ptr<Renderer::RenderTexture>> shadowMaps;
		shadowMaps.reserve(core->GetLightManager()->GetShadowCascades().size());
		std::vector<glm::mat4> shadowMatrices;
		shadowMatrices.reserve(core->GetLightManager()->GetShadowCascades().size());

		for (const ShadowCascade& cascade : core->GetLightManager()->GetShadowCascades())
		{
			shadowMaps.emplace_back(cascade.renderTexture);
			shadowMatrices.emplace_back(cascade.lightSpaceMatrix);
		}

		mShader->mShader->uniform("u_ShadowMaps", shadowMaps, 20);
		mShader->mShader->uniform("u_LightSpaceMatrices", shadowMatrices);
        
		std::vector<Renderer::Texture*> rawTextures;
		rawTextures.reserve(mTextures.size());
		for (const auto& tex : mTextures)
		{
			rawTextures.emplace_back(tex->mTexture.get());
		}

		// Call the updated draw function with support for multi-materials
		mShader->mShader->draw(mModel->mModel.get(), rawTextures);
	}

	void ModelRenderer::OnShadowRender(const glm::mat4& _lightSpaceMatrix)
	{
		// Don't draw static models each frame in the shadow pass
		if (mPreBakeShadows || !mModel)
			return;

		glm::mat4 entityModel = GetEntity()->GetComponent<Transform>()->GetModel();

		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(mRotationOffset.x), glm::vec3(1, 0, 0)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(mRotationOffset.y), glm::vec3(0, 1, 0)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(mRotationOffset.z), glm::vec3(0, 0, 1));
		glm::mat4 offsetMatrix = glm::translate(glm::mat4(1.0f), mPositionOffset) * rotationMatrix;

		glm::mat4 model = entityModel * offsetMatrix;

		mDepthShader->mShader->uniform("u_Model", model);

		mDepthShader->mShader->uniform("u_LightSpaceMatrix", _lightSpaceMatrix);
		mDepthShader->mShader->uniform("u_AlphaCutoff", mAlphaCutoff);


		std::vector<Renderer::Texture*> rawTextures;
		rawTextures.reserve(mTextures.size());
		for (const auto& tex : mTextures)
		{
			rawTextures.emplace_back(tex->mTexture.get());
		}

		// Call the updated draw function with support for multi-materials
		mDepthShader->mShader->draw(mModel->mModel.get(), rawTextures);
	}

}