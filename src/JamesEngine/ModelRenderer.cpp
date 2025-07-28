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

		if (!mSplitPrebakedShadowMap)
		{
			// Prebake a shadow map for static models
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDisable(GL_MULTISAMPLE);
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

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

			// Compute light-space matrix
			glm::mat4 lightSpaceMatrix = lightProjection * lightView;

			// Get maximum texture size for render textures
			GLint maxTextureSize = 0;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

			float vram = GetCore()->GetWindow()->GetVRAMGB();
			vram = 6;

			// Reference point: 8GB VRAM = 16K textures (for 1 map) or 8K (for 4 maps)
			float referenceVRAM = 8.0f;
			float referenceSize = 16384.0f;

			// Calculate raw size with square root scaling (better than linear)
			// Square root scaling gives better distribution across VRAM sizes
			float scaleFactor = sqrt(vram / referenceVRAM);
			float rawSize = referenceSize * scaleFactor;

			// Find nearest power of 2 (required for optimal texture performance)
			int powerOf2 = 1;
			while (powerOf2 * 2 <= rawSize)
				powerOf2 *= 2;

			// Enforce minimum and maximum limits
			int minSize = 1024; // Minimum quality threshold

			maxTextureSize = std::min(std::max(powerOf2, minSize), maxTextureSize);

			std::shared_ptr<Renderer::RenderTexture> renderTexture = std::make_shared<Renderer::RenderTexture>(maxTextureSize, maxTextureSize, Renderer::RenderTextureType::Depth);

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

			std::cout << "Pre-baked shadow map for " << GetEntity()->GetTag() << " with a texture size of " << maxTextureSize << std::endl;
		}
		else
		{
			std::cout << "Pre-baking shadows in 4 quadrants for " << GetEntity()->GetTag() << std::endl;

			// Prebake shadow maps for static models
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDisable(GL_MULTISAMPLE);
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

			glm::vec2 fullOrthoSize;

			if (mCustomShadowMapSize.x <= 0 || mCustomShadowMapSize.y <= 0)
				fullOrthoSize = glm::vec2(mModel->mModel->get_width() / 2, mModel->mModel->get_length() / 2);
			else
				fullOrthoSize = glm::vec2(mCustomShadowMapSize.x , mCustomShadowMapSize.y );

			// Calculate quadrant-specific parameters
			glm::vec2 quadrantOrthoSize = fullOrthoSize / 2.0f;
			float nearPlane = 0.1f;
			float farPlane = 1000.f;
			glm::vec3 sceneCenter = GetPosition() + mPositionOffset + mCustomCenter;
			glm::vec3 lightDir = glm::normalize(GetCore()->GetLightManager()->GetDirectionalLightDirection());

			// Get maximum texture size for render textures
			GLint maxTextureSize = 0;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

			float vram = GetCore()->GetWindow()->GetVRAMGB();

			// Reference point: 8GB VRAM = 16K textures (for 1 map) or 8K (for 4 maps)
			float referenceVRAM = 8.0f;
			float referenceSize = 16384.0f;

			// Calculate raw size with square root scaling (better than linear)
			// Square root scaling gives better distribution across VRAM sizes
			float scaleFactor = sqrt(vram / referenceVRAM);
			float rawSize = referenceSize * scaleFactor;

			// Find nearest power of 2 (required for optimal texture performance)
			int powerOf2 = 1;
			while (powerOf2 * 2 <= rawSize)
				powerOf2 *= 2;

			// Enforce minimum and maximum limits
			int minSize = 1024; // Minimum quality threshold

			maxTextureSize = std::min(std::max(powerOf2, minSize), maxTextureSize);

			// Quadrant offsets (centered on each quarter of the model)
			glm::vec2 quadrantOffsets[4] = {
				glm::vec2(-quadrantOrthoSize.x, -quadrantOrthoSize.y),  // Bottom-left
				glm::vec2(quadrantOrthoSize.x, -quadrantOrthoSize.y),  // Bottom-right
				glm::vec2(-quadrantOrthoSize.x,  quadrantOrthoSize.y),  // Top-left
				glm::vec2(quadrantOrthoSize.x,  quadrantOrthoSize.y)   // Top-right
			};

			// Create 4 shadow maps, one for each quadrant
			for (int i = 0; i < 4; i++)
			{
				// Calculate center point with overlap (25% overlap between adjacent quadrants)
				float overlapFactor = 0.75f; // Controls how much each quadrant extends toward the center (lower = more overlap)
				glm::vec3 quadrantCenter = sceneCenter + glm::vec3(quadrantOffsets[i].x * overlapFactor, 0.0f, quadrantOffsets[i].y * overlapFactor);


				// Calculate light position and view matrix for this quadrant
				glm::vec3 lightPos = quadrantCenter - lightDir * (farPlane / 2.f);
				glm::mat4 lightView = glm::lookAt(lightPos, quadrantCenter, glm::vec3(0, 1, 0));

				// Create projection matrix for this quadrant
				glm::mat4 lightProjection = glm::ortho(
					-quadrantOrthoSize.x, quadrantOrthoSize.x,
					-quadrantOrthoSize.y, quadrantOrthoSize.y,
					nearPlane, farPlane
				);

				// Compute light-space matrix
				glm::mat4 lightSpaceMatrix = lightProjection * lightView;

				// Create render texture for this quadrant
				std::shared_ptr<Renderer::RenderTexture> renderTexture =
					std::make_shared<Renderer::RenderTexture>(maxTextureSize, maxTextureSize, Renderer::RenderTextureType::Depth);

				renderTexture->clear();
				renderTexture->bind();
				glViewport(0, 0, renderTexture->getWidth(), renderTexture->getHeight());

				mPreBakeShadows = false; // Disable pre-baking shadows during render

				// Render shadows for this quadrant
				OnShadowRender(lightSpaceMatrix);

				std::cout << "Pre-baked shadow map for quadrant " << i << " with a texture size of " << maxTextureSize << std::endl;

				mPreBakeShadows = true; // Re-enable for future renders

				renderTexture->unbind();

				// Add this shadow map to the light manager
				GetCore()->GetLightManager()->AddPreBakedShadowMap(renderTexture, lightSpaceMatrix);

				glFlush();
				glFinish();
			}

			// Restore GL state
			glEnable(GL_CULL_FACE);
			glEnable(GL_BLEND);
			glEnable(GL_MULTISAMPLE);
			glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		}
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