#include "SceneRenderer.h"

#include "Core.h"

#include "Resources.h"
#include "Camera.h"
#include "Transform.h"
#include "Skybox.h"
#include "Timer.h"

#include <algorithm>

namespace JamesEngine
{

	SceneRenderer::SceneRenderer(std::shared_ptr<Core> _core)
		: mCore(_core)
	{
		mObjShader = mCore.lock()->GetResources()->Load<Shader>("shaders/ObjShader");
		mDepthShader = mCore.lock()->GetResources()->Load<Shader>("shaders/DepthOnly");
		mDepthAlphaShader = mCore.lock()->GetResources()->Load<Shader>("shaders/DepthOnlyAlpha");
		mSSAOShader = mCore.lock()->GetResources()->Load<Shader>("shaders/SSAOShader");
		mBlurShader = mCore.lock()->GetResources()->Load<Shader>("shaders/BlurShader");

		// Size of 1,1 just to initialize
		mShadingPass = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::ColourAndDepth);
		mDepthPass = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::Depth);
		mAORaw = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::PostProcessTarget);
		mAOIntermediate = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::PostProcessTarget);
		mAOBlurred = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::PostProcessTarget);

		Renderer::Face face;
		face.a.m_position = glm::vec3(1.0f, 0.0f, 0.0f);
		face.b.m_position = glm::vec3(0.0f, 1.0f, 0.0f);
		face.c.m_position = glm::vec3(0.0f, 0.0f, 0.0f);
		face.a.m_texcoords = glm::vec2(1.0f, 0.0f);
		face.b.m_texcoords = glm::vec2(0.0f, 1.0f);
		face.c.m_texcoords = glm::vec2(0.0f, 0.0f);
		mRect->add(face);

		Renderer::Face face2;
		face2.a.m_position = glm::vec3(1.0f, 0.0f, 0.0f);
		face2.b.m_position = glm::vec3(1.0f, 1.0f, 0.0f);
		face2.c.m_position = glm::vec3(0.0f, 1.0f, 0.0f);
		face2.a.m_texcoords = glm::vec2(1.0f, 0.0f);
		face2.b.m_texcoords = glm::vec2(1.0f, 1.0f);
		face2.c.m_texcoords = glm::vec2(0.0f, 1.0f);
		mRect->add(face2);
	}

	void SceneRenderer::RenderScene()
	{
		//ScopedTimer timer("SceneRenderer::RenderScene");

		auto core = mCore.lock();
		auto window = core->GetWindow();
		auto camera = core->GetCamera();

		int winW, winH;
		window->GetWindowSize(winW, winH);

		if (winW != mLastViewportSize.x || winH != mLastViewportSize.y)
		{
			// Resize textures
			mShadingPass->resize(winW, winH);
			mDepthPass->resize(winW, winH);
			mAORaw->resize(winW * mSSAOResultionScale, winH * mSSAOResultionScale);
			mAOIntermediate->resize(winW * mSSAOResultionScale, winH * mSSAOResultionScale);
			mAOBlurred->resize(winW * mSSAOResultionScale, winH * mSSAOResultionScale);

			mLastViewportSize = glm::ivec2(winW, winH);
		}

		// Global uniforms

		mObjShader->mShader->use();
		mObjShader->mShader->uniform("u_Projection", camera->GetProjectionMatrix());
		std::vector<std::shared_ptr<Camera>> cams;
		core->FindComponents<Camera>(cams); // FOR DEBUGGING - can see the scene from one veiw while still processes it from another (I know the first camera is a free roam cam)
		mObjShader->mShader->uniform("u_View", cams.at(0)->GetViewMatrix());
		mObjShader->mShader->uniform("u_ViewPos", cams.at(0)->GetPosition());
		mObjShader->mShader->uniform("u_DebugVP", camera->GetProjectionMatrix() * camera->GetViewMatrix());
		mObjShader->mShader->uniform("u_View", camera->GetViewMatrix());
		mObjShader->mShader->uniform("u_ViewPos", camera->GetPosition());
		mObjShader->mShader->unuse();

		if (!core->mLightManager->GetShadowCascades().empty())
		{
			// --- Render-state for depth-only shadow pass ---
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDisable(GL_MULTISAMPLE);
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

			// Camera info
			const glm::vec3 camPos = camera->GetPosition();
			const glm::vec3 camFwd = -camera->GetEntity()->GetComponent<Transform>()->GetForward();
			const glm::vec3 camUp = camera->GetEntity()->GetComponent<Transform>()->GetUp();
			const glm::vec3 camRight = camera->GetEntity()->GetComponent<Transform>()->GetRight();
			const float vfov = glm::radians(camera->GetFov());
			const float aspect = (winH > 0) ? (static_cast<float>(winW) / static_cast<float>(winH)) : (16.0f / 9.0f);
			const float camNear = camera->GetNearClip();
			const float camFar = camera->GetFarClip();
			const glm::mat4 viewMat = camera->GetViewMatrix();
			const glm::mat4 projMat = camera->GetProjectionMatrix();

			const glm::vec3 lightDir = glm::normalize(core->mLightManager->GetDirectionalLightDirection());

			auto& cascades = core->mLightManager->GetShadowCascades();
			const int numCascades = (int)cascades.size();

			float eyeDist = 10.f;
			float ZMargin = 100.f;

			// SHADOW CASCADE RENDERING
			for (int ci = 0; ci < numCascades; ++ci)
			{
				ShadowCascade& cascade = cascades[ci];

				float nearPlane = cascade.splitDepths.x;
				if (nearPlane < camNear) nearPlane = camNear;
				float farPlane = cascade.splitDepths.y;
				if (farPlane > camFar) farPlane = camFar;

				auto cascadeProj = glm::perspective(
					vfov,
					aspect,
					nearPlane,
					farPlane
				);

				auto inv = glm::inverse(cascadeProj * viewMat);

				std::vector<glm::vec4> frustumCorners;
				for (unsigned int x = 0; x < 2; ++x)
				{
					for (unsigned int y = 0; y < 2; ++y)
					{
						for (unsigned int z = 0; z < 2; ++z)
						{
							const glm::vec4 pt =
								inv * glm::vec4(
									2.0f * x - 1.0f,
									2.0f * y - 1.0f,
									2.0f * z - 1.0f,
									1.0f);
							frustumCorners.push_back(pt / pt.w);
						}
					}
				}

				glm::vec3 center = glm::vec3(0, 0, 0);
				for (const auto& v : frustumCorners)
				{
					center += glm::vec3(v);
				}
				center /= frustumCorners.size();

				auto lightView = glm::lookAt(
					center - lightDir * eyeDist,
					center,
					glm::vec3(0.0f, 1.0f, 0.0f)
				);

				float minX = std::numeric_limits<float>::max();
				float maxX = std::numeric_limits<float>::lowest();
				float minY = std::numeric_limits<float>::max();
				float maxY = std::numeric_limits<float>::lowest();
				float minZ = std::numeric_limits<float>::max();
				float maxZ = std::numeric_limits<float>::lowest();
				for (const auto& v : frustumCorners)
				{
					const auto trf = lightView * v;
					minX = std::min(minX, trf.x);
					maxX = std::max(maxX, trf.x);
					minY = std::min(minY, trf.y);
					maxY = std::max(maxY, trf.y);
					minZ = std::min(minZ, trf.z);
					maxZ = std::max(maxZ, trf.z);
				}

				minZ -= ZMargin;
				maxZ += ZMargin;

				glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);

				cascade.lightSpaceMatrix = lightProj * lightView;

				mDepthShader->mShader->use();
				mDepthShader->mShader->uniform("u_View", lightView);
				mDepthShader->mShader->uniform("u_Projection", lightProj);
				mDepthShader->mShader->unuse();

				mDepthAlphaShader->mShader->use();
				mDepthAlphaShader->mShader->uniform("u_View", lightView);
				mDepthAlphaShader->mShader->uniform("u_Projection", lightProj);
				mDepthAlphaShader->mShader->unuse();

				// Render this cascade
				cascade.renderTexture->clear();
				cascade.renderTexture->bind();
				glViewport(0, 0, cascade.renderTexture->getWidth(), cascade.renderTexture->getHeight());

				// Build culled list of shadow-casting materials
				auto culledShadowMaterials = FrustumCulledMaterials(
												mShadowMaterials,
												lightView,
												lightProj);

				// Avoids copying or double-deleting the underlying GL object.
				auto asShared = [](const Renderer::Texture& t) -> std::shared_ptr<Renderer::Texture> {
					return std::shared_ptr<Renderer::Texture>(const_cast<Renderer::Texture*>(&t), [](Renderer::Texture*) {});
					};

				// Render all shadow-casting objects
				for (const auto& shadowMaterial : culledShadowMaterials)
				{
					const auto& materialGroup = shadowMaterial.materialGroup;
					const auto& pbr = materialGroup.pbr;
					const auto& embedded = shadowMaterial.model->mModel->GetEmbeddedTextures();

					// Culling per material (same as before)
					GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);
					if (pbr.doubleSided) glDisable(GL_CULL_FACE);
					else glEnable(GL_CULL_FACE);

					if (pbr.alphaMode != Renderer::Model::PBRMaterial::AlphaMode::AlphaOpaque)
					{
						mDepthAlphaShader->mShader->use();

						mDepthAlphaShader->mShader->uniform("u_Model", shadowMaterial.transform);

						// Upload base texture because may have transparent alpha
						if (pbr.baseColorTexIndex >= 0 && pbr.baseColorTexIndex < (int)embedded.size())
						{
							mDepthAlphaShader->mShader->uniform("u_AlbedoMap", asShared(embedded[pbr.baseColorTexIndex]), 0);
						}

						mDepthAlphaShader->mShader->uniform("u_AlphaCutoff", pbr.alphaCutoff);

						// Draw this material only
						const GLsizei vertCount = static_cast<GLsizei>(materialGroup.faces.size() * 3);
						mDepthAlphaShader->mShader->draw(materialGroup.vao, vertCount);

						mDepthAlphaShader->mShader->unuse();
					}
					else
					{
						mDepthShader->mShader->use();

						mDepthShader->mShader->uniform("u_Model", shadowMaterial.transform);

						const GLsizei vertCount = static_cast<GLsizei>(materialGroup.faces.size() * 3);
						mDepthShader->mShader->draw(materialGroup.vao, vertCount);

						mDepthShader->mShader->unuse();
					}

					// Restore culling
					if (prevCullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
				}

				cascade.renderTexture->unbind();
			}

			window->ResetGLModes();

			// Want to upload shadow maps at the start, but it needs to be done after the shadow maps are rendered
			std::vector<std::shared_ptr<Renderer::RenderTexture>> shadowMaps;
			shadowMaps.reserve(core->mLightManager->GetShadowCascades().size());
			std::vector<glm::mat4> shadowMatrices;
			shadowMatrices.reserve(core->mLightManager->GetShadowCascades().size());

			for (const ShadowCascade& cascade : core->mLightManager->GetShadowCascades())
			{
				shadowMaps.emplace_back(cascade.renderTexture);
				shadowMatrices.emplace_back(cascade.lightSpaceMatrix);
			}

			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_NumCascades", (int)shadowMaps.size());
			mObjShader->mShader->uniform("u_ShadowMaps", shadowMaps, 20);
			mObjShader->mShader->uniform("u_LightSpaceMatrices", shadowMatrices);
			mObjShader->mShader->unuse();
		}
		else
		{
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_NumCascades", 0);
			mObjShader->mShader->unuse();
		}

		// Prepare window for rendering
		window->Update();
		window->ClearWindow();

		window->ResetGLModes();

		// Build a copy sorted by distance (closest -> furthest)
		const glm::mat4 camView = camera->GetViewMatrix();
		const glm::mat4 camProj = camera->GetProjectionMatrix();
		const glm::vec3 camPos = camera->GetPosition();
		const glm::mat4 VP = camProj * camView;


		std::vector<MaterialRenderInfo> distanceSortedOpaqueMaterials = FrustumCulledDistanceSortedMaterials(
																			mOpaqueMaterials,
																			camView,
																			camProj,
																			camPos,
																			DepthSortMode::FrontToBack);

		std::vector<MaterialRenderInfo> distanceSortedTransparentMaterials = FrustumCulledDistanceSortedMaterials(
																				mTransparentMaterials,
																				camView,
																				camProj,
																				camPos,
																				DepthSortMode::BackToFront);

		// This avoids copying or double-deleting the underlying GL object.
		auto asShared = [](const Renderer::Texture& t) -> std::shared_ptr<Renderer::Texture> {
			return std::shared_ptr<Renderer::Texture>(const_cast<Renderer::Texture*>(&t), [](Renderer::Texture*) {}); // no-op deleter
			};

		// DEPTH PASS
		mDepthPass->clear();
		mDepthPass->bind();
		glViewport(0, 0, mDepthPass->getWidth(), mDepthPass->getHeight());

		// Depth-only state
		glDisable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDisable(GL_MULTISAMPLE);
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

		// OPAQUES (front-to-back already)
		for (const auto& opaqueMaterial : distanceSortedOpaqueMaterials)
		{
			const auto& pbr = opaqueMaterial.materialGroup.pbr;
			GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);
			if (pbr.doubleSided) glDisable(GL_CULL_FACE);
			else glEnable(GL_CULL_FACE);

			if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaOpaque)
			{
				mDepthShader->mShader->use();
				// Reuse the same uniform name the depth shader expects
				mDepthShader->mShader->uniform("u_View", camView);
				mDepthShader->mShader->uniform("u_Projection", camProj);
				mDepthShader->mShader->uniform("u_Model", opaqueMaterial.transform);

				const GLsizei vertCount = static_cast<GLsizei>(opaqueMaterial.materialGroup.faces.size() * 3);
				mDepthShader->mShader->draw(opaqueMaterial.materialGroup.vao, vertCount);
				mDepthShader->mShader->unuse();
			}
			else
			{
				mDepthAlphaShader->mShader->use();
				mDepthAlphaShader->mShader->uniform("u_View", camView);
				mDepthAlphaShader->mShader->uniform("u_Projection", camProj);
				mDepthAlphaShader->mShader->uniform("u_Model", opaqueMaterial.transform);
				mDepthAlphaShader->mShader->uniform("u_AlphaCutoff", pbr.alphaCutoff);

				// BaseColor for alpha test if present
				const auto& embedded = opaqueMaterial.model->mModel->GetEmbeddedTextures();
				if (pbr.baseColorTexIndex >= 0 && pbr.baseColorTexIndex < (int)embedded.size())
				{
					mDepthAlphaShader->mShader->uniform("u_AlbedoMap", asShared(embedded[pbr.baseColorTexIndex]), 0);
				}

				const GLsizei vertCount = static_cast<GLsizei>(opaqueMaterial.materialGroup.faces.size() * 3);
				mDepthAlphaShader->mShader->draw(opaqueMaterial.materialGroup.vao, vertCount);
				mDepthAlphaShader->mShader->unuse();
			}

			if (prevCullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		}

		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		for (const auto& transparentMaterial : distanceSortedTransparentMaterials)
		{
			const auto& pbr = transparentMaterial.materialGroup.pbr;

			GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);
			if (pbr.doubleSided) glDisable(GL_CULL_FACE);
			else glEnable(GL_CULL_FACE);

			mDepthAlphaShader->mShader->use();
			mDepthAlphaShader->mShader->uniform("u_View", camView);
			mDepthAlphaShader->mShader->uniform("u_Projection", camProj);
			mDepthAlphaShader->mShader->uniform("u_Model", transparentMaterial.transform);
			mDepthAlphaShader->mShader->uniform("u_AlphaCutoff", pbr.alphaCutoff);

			// BaseColor for alpha test if present
			const auto& embedded = transparentMaterial.model->mModel->GetEmbeddedTextures();
			if (pbr.baseColorTexIndex >= 0 && pbr.baseColorTexIndex < (int)embedded.size())
			{
				mDepthAlphaShader->mShader->uniform("u_AlbedoMap", asShared(embedded[pbr.baseColorTexIndex]), 0);
			}

			const GLsizei vertCount = static_cast<GLsizei>(transparentMaterial.materialGroup.faces.size() * 3);
			mDepthAlphaShader->mShader->draw(transparentMaterial.materialGroup.vao, vertCount);
			mDepthAlphaShader->mShader->unuse();

			if (prevCullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		}

		mDepthPass->unbind();

		glDisable(GL_BLEND);

		// Restore color writes
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// SSAO PASS
		// Raw AO
		mAORaw->clear();
		mAORaw->bind();
		mSSAOShader->mShader->use();
		mSSAOShader->mShader->uniform("u_Depth", mDepthPass);
		mSSAOShader->mShader->uniform("u_Proj", camProj);
		mSSAOShader->mShader->uniform("u_InvView", glm::inverse(camView));
		mSSAOShader->mShader->uniform("u_InvProj", glm::inverse(camProj));
		mSSAOShader->mShader->uniform("u_InvResolution", glm::vec2(1.0f / mDepthPass->getWidth(), 1.0f / mDepthPass->getHeight()));
		mSSAOShader->mShader->uniform("u_Radius", mSSAORadius);
		mSSAOShader->mShader->uniform("u_Bias", mSSAOBias);
		mSSAOShader->mShader->uniform("u_Power", mSSAOPower);
		mSSAOShader->mShader->draw(mRect.get());
		mSSAOShader->mShader->unuse();
		mAORaw->unbind();

		// Blur AO - horizontal
		mAOIntermediate->clear();
		mAOIntermediate->bind();
		mBlurShader->mShader->use();
		mBlurShader->mShader->uniform("u_RawAO", mAORaw);
		mBlurShader->mShader->uniform("u_InvResolution", glm::vec2(1.0f / mAORaw->getWidth(), 1.0f / mAORaw->getHeight()));
		mBlurShader->mShader->uniform("u_Direction", glm::vec2(1, 0));
		mBlurShader->mShader->draw(mRect.get());
		mAOIntermediate->unbind();

		// Blur AO - vertical
		mAOBlurred->clear();
		mAOBlurred->bind();
		mBlurShader->mShader->use();
		mBlurShader->mShader->uniform("u_RawAO", mAOIntermediate);
		mBlurShader->mShader->uniform("u_Direction", glm::vec2(0, 1));
		mBlurShader->mShader->draw(mRect.get());
		mAOBlurred->unbind();

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		mShadingPass->clear();
		mShadingPass->bind();
		glViewport(0, 0, mShadingPass->getWidth(), mShadingPass->getHeight());

		core->mSkybox->RenderSkybox();

		mObjShader->mShader->use();

		mObjShader->mShader->uniform("u_SSAO", mAOBlurred, 27);
		mObjShader->mShader->uniform("u_AOStrength", mAOStrength);
		mObjShader->mShader->uniform("u_AOSpecScale", mAOSpecScale);
		mObjShader->mShader->uniform("u_AOMin", mAOMin);
		mObjShader->mShader->uniform("u_InvColorResolution", glm::vec2(1.f/mShadingPass->getWidth(), 1.f/mShadingPass->getHeight()));

		mObjShader->mShader->uniform("u_PCSSBase", mPCSSBase);
		mObjShader->mShader->uniform("u_PCSSScale", mPCSSScale);

		// Fallbacks
		mObjShader->mShader->uniform("u_AlbedoFallback", mBaseColorStrength);
		mObjShader->mShader->uniform("u_MetallicFallback", mMetallicness);
		mObjShader->mShader->uniform("u_RoughnessFallback", mRoughness);
		mObjShader->mShader->uniform("u_AOFallback", mAOFallback);
		mObjShader->mShader->uniform("u_EmissiveFallback", mEmmisive);

		// SHADING PASS
		for (const auto& opaqueMaterial : distanceSortedOpaqueMaterials)
		{
			std::shared_ptr<Model> modelToRender = opaqueMaterial.model;

			const auto& pbr = opaqueMaterial.materialGroup.pbr;
			const auto& embedded = opaqueMaterial.model->mModel->GetEmbeddedTextures();

			mObjShader->mShader->uniform("u_Model", opaqueMaterial.transform);

			// Albedo (unit 0)
			if (pbr.baseColorTexIndex >= 0 && pbr.baseColorTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_AlbedoMap", asShared(embedded[pbr.baseColorTexIndex]), 0);
				mObjShader->mShader->uniform("u_HasAlbedoMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasAlbedoMap", 0);
			}

			// Normal (unit 1)
			if (pbr.normalTexIndex >= 0 && pbr.normalTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_NormalMap", asShared(embedded[pbr.normalTexIndex]), 1);
				mObjShader->mShader->uniform("u_HasNormalMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasNormalMap", 0);
			}

			// Metallic-Roughness (unit 2)
			if (pbr.metallicRoughnessTexIndex >= 0 && pbr.metallicRoughnessTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_MetallicRoughnessMap", asShared(embedded[pbr.metallicRoughnessTexIndex]), 2);
				mObjShader->mShader->uniform("u_HasMetallicRoughnessMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasMetallicRoughnessMap", 0);
			}

			// Occlusion (unit 3)
			if (pbr.occlusionTexIndex >= 0 && pbr.occlusionTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_OcclusionMap", asShared(embedded[pbr.occlusionTexIndex]), 3);
				mObjShader->mShader->uniform("u_HasOcclusionMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasOcclusionMap", 0);
			}

			// Emissive (unit 4)
			if (pbr.emissiveTexIndex >= 0 && pbr.emissiveTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_EmissiveMap", asShared(embedded[pbr.emissiveTexIndex]), 4);
				mObjShader->mShader->uniform("u_HasEmissiveMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasEmissiveMap", 0);
			}

			// Transmission (unit 5)
			if (pbr.transmissionTexIndex >= 0 && pbr.transmissionTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_TransmissionTex", asShared(embedded[pbr.transmissionTexIndex]), 5);
				mObjShader->mShader->uniform("u_HasTransmissionTex", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasTransmissionTex", 0);
			}

			// Material factors
			mObjShader->mShader->uniform("u_BaseColorFactor", pbr.baseColorFactor);
			mObjShader->mShader->uniform("u_MetallicFactor", pbr.metallicFactor);
			mObjShader->mShader->uniform("u_RoughnessFactor", pbr.roughnessFactor);
			mObjShader->mShader->uniform("u_NormalScale", pbr.normalScale);
			mObjShader->mShader->uniform("u_OcclusionStrength", pbr.occlusionStrength);
			mObjShader->mShader->uniform("u_EmissiveFactor", pbr.emissiveFactor);
			mObjShader->mShader->uniform("u_TransmissionFactor", pbr.transmissionFactor);
			mObjShader->mShader->uniform("u_IOR", pbr.ior);

			// Alpha
			int alphaMode = 0;
			if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaMask)  alphaMode = 1;
			if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaBlend) alphaMode = 2;
			mObjShader->mShader->uniform("u_AlphaMode", alphaMode);
			mObjShader->mShader->uniform("u_AlphaCutoff", pbr.alphaCutoff);

			if (pbr.doubleSided) glDisable(GL_CULL_FACE);
			else glEnable(GL_CULL_FACE);

			// Draw this material only
			const GLsizei vertCount = static_cast<GLsizei>(opaqueMaterial.materialGroup.faces.size() * 3);
			mObjShader->mShader->draw(opaqueMaterial.materialGroup.vao, vertCount);
		}

		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		// THEN RENDER TRANSPARENT MATERIALS
		for (const auto& transparentMaterial : distanceSortedTransparentMaterials)
		{
			std::shared_ptr<Model> modelToRender = transparentMaterial.model;

			const auto& pbr = transparentMaterial.materialGroup.pbr;
			const auto& embedded = transparentMaterial.model->mModel->GetEmbeddedTextures();

			mObjShader->mShader->uniform("u_Model", transparentMaterial.transform);

			// Albedo (unit 0)
			if (pbr.baseColorTexIndex >= 0 && pbr.baseColorTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_AlbedoMap", asShared(embedded[pbr.baseColorTexIndex]), 0);
				mObjShader->mShader->uniform("u_HasAlbedoMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasAlbedoMap", 0);
			}

			// Normal (unit 1)
			if (pbr.normalTexIndex >= 0 && pbr.normalTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_NormalMap", asShared(embedded[pbr.normalTexIndex]), 1);
				mObjShader->mShader->uniform("u_HasNormalMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasNormalMap", 0);
			}

			// Metallic-Roughness (unit 2)
			if (pbr.metallicRoughnessTexIndex >= 0 && pbr.metallicRoughnessTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_MetallicRoughnessMap", asShared(embedded[pbr.metallicRoughnessTexIndex]), 2);
				mObjShader->mShader->uniform("u_HasMetallicRoughnessMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasMetallicRoughnessMap", 0);
			}

			// Occlusion (unit 3)
			if (pbr.occlusionTexIndex >= 0 && pbr.occlusionTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_OcclusionMap", asShared(embedded[pbr.occlusionTexIndex]), 3);
				mObjShader->mShader->uniform("u_HasOcclusionMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasOcclusionMap", 0);
			}

			// Emissive (unit 4)
			if (pbr.emissiveTexIndex >= 0 && pbr.emissiveTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_EmissiveMap", asShared(embedded[pbr.emissiveTexIndex]), 4);
				mObjShader->mShader->uniform("u_HasEmissiveMap", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasEmissiveMap", 0);
			}

			// Transmission (unit 5)
			if (pbr.transmissionTexIndex >= 0 && pbr.transmissionTexIndex < (int)embedded.size())
			{
				mObjShader->mShader->uniform("u_TransmissionTex", asShared(embedded[pbr.transmissionTexIndex]), 5);
				mObjShader->mShader->uniform("u_HasTransmissionTex", 1);
			}
			else
			{
				mObjShader->mShader->uniform("u_HasTransmissionTex", 0);
			}

			// Material factors
			mObjShader->mShader->uniform("u_BaseColorFactor", pbr.baseColorFactor);
			mObjShader->mShader->uniform("u_MetallicFactor", pbr.metallicFactor);
			mObjShader->mShader->uniform("u_RoughnessFactor", pbr.roughnessFactor);
			mObjShader->mShader->uniform("u_NormalScale", pbr.normalScale);
			mObjShader->mShader->uniform("u_OcclusionStrength", pbr.occlusionStrength);
			mObjShader->mShader->uniform("u_EmissiveFactor", pbr.emissiveFactor);
			mObjShader->mShader->uniform("u_TransmissionFactor", pbr.transmissionFactor);
			mObjShader->mShader->uniform("u_IOR", pbr.ior);

			// Alpha
			int alphaMode = 0;
			if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaMask)  alphaMode = 1;
			if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaBlend) alphaMode = 2;
			mObjShader->mShader->uniform("u_AlphaMode", alphaMode);
			mObjShader->mShader->uniform("u_AlphaCutoff", pbr.alphaCutoff);

			if (pbr.doubleSided) glDisable(GL_CULL_FACE);
			else glEnable(GL_CULL_FACE);

			// Draw this material only
			const GLsizei vertCount = static_cast<GLsizei>(transparentMaterial.materialGroup.faces.size() * 3);
			mObjShader->mShader->draw(transparentMaterial.materialGroup.vao, vertCount);
		}

		mObjShader->mShader->unuse();

		mShadingPass->unbind();

		// Just blit the color buffer to the default framebuffer, temporary
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mShadingPass->getFBO());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		int w = mShadingPass->getWidth(), h = mShadingPass->getHeight();
		glBlitFramebuffer(0, 0, w, h, 0, 0, winW, winH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);


		window->ResetGLModes();

		ClearScene();
	}

	void SceneRenderer::AddModel(std::shared_ptr<Model> _model, const glm::mat4& _transform, const ShadowOverride& _shadow)
	{
		SubmissionInfo info;
		info.model = _model;
		info.transform = _transform;
		info.shadowMode = _shadow.mode;
		info.shadowModel = _shadow.proxy;

		if (info.shadowMode == ShadowMode::Proxy && !info.shadowModel)
			info.shadowMode = ShadowMode::Default; // User said to use proxy but didn't provide one, fallback to default

		mSubmissions.push_back(info);

		auto opaque = Renderer::Model::PBRMaterial::AlphaMode::AlphaOpaque;
		auto mask = Renderer::Model::PBRMaterial::AlphaMode::AlphaMask;
		auto blend = Renderer::Model::PBRMaterial::AlphaMode::AlphaBlend;

		for (const auto& materialGroup : _model->mModel->GetMaterialGroups())
		{
			const auto& pbr = materialGroup.pbr;

			if (pbr.alphaMode == opaque || pbr.alphaMode == mask)
			{
				mOpaqueMaterials.push_back({ const_cast<Renderer::Model::MaterialGroup&>(materialGroup), _model, _transform });
			}
			else if (pbr.alphaMode == blend)
			{
				mTransparentMaterials.push_back({ const_cast<Renderer::Model::MaterialGroup&>(materialGroup), _model, _transform });
			}

			if (info.shadowMode != ShadowMode::Proxy)
			{
				mShadowMaterials.push_back({ const_cast<Renderer::Model::MaterialGroup&>(materialGroup), _model, _transform });
			}
		}

		if (info.shadowMode == ShadowMode::Proxy)
		{
			for (const auto& materialGroup : info.shadowModel->mModel->GetMaterialGroups())
			{
				mShadowMaterials.push_back({ const_cast<Renderer::Model::MaterialGroup&>(materialGroup), info.shadowModel, _transform });
			}
		}
	}

	void SceneRenderer::ClearScene()
	{
		mSubmissions.clear();
		mOpaqueMaterials.clear();
		mTransparentMaterials.clear();
		mShadowMaterials.clear();
	}

	std::vector<MaterialRenderInfo> SceneRenderer::FrustumCulledMaterials(
		const std::vector<MaterialRenderInfo>& _materials,
		const glm::mat4& _view,
		const glm::mat4& _proj)
	{
		struct Plane
		{
			glm::vec3 n;
			float d; // plane: n·x + d >= 0 => inside
		};

		const glm::mat4 VP = _proj * _view;

		// Extract rows from column-major matrix
		auto getRow = [&](int r)
			{
				return glm::vec4(VP[0][r], VP[1][r], VP[2][r], VP[3][r]);
			};

		const glm::vec4 row0 = getRow(0);
		const glm::vec4 row1 = getRow(1);
		const glm::vec4 row2 = getRow(2);
		const glm::vec4 row3 = getRow(3);

		Plane planes[6] = {
			{ glm::vec3(row3 + row0), (row3 + row0).w }, // Left
			{ glm::vec3(row3 - row0), (row3 - row0).w }, // Right
			{ glm::vec3(row3 + row1), (row3 + row1).w }, // Bottom
			{ glm::vec3(row3 - row1), (row3 - row1).w }, // Top
			{ glm::vec3(row3 + row2), (row3 + row2).w }, // Near
			{ glm::vec3(row3 - row2), (row3 - row2).w }, // Far
		};

		// Normalise planes
		for (Plane& p : planes)
		{
			float invLen = 1.0f / glm::length(p.n);
			p.n *= invLen;
			p.d *= invLen;
		}

		std::vector<MaterialRenderInfo> result;
		result.reserve(_materials.size());

		for (size_t i = 0; i < _materials.size(); ++i)
		{
			const auto& it = _materials[i];

			const glm::mat4  M = it.transform;
			const glm::vec3  e = it.materialGroup.boundsHalfExtentsMS;
			const glm::vec3  cMS = it.materialGroup.boundsCenterMS;
			const glm::vec3  cW = glm::vec3(M * glm::vec4(cMS, 1.0f));
			const glm::mat3  A = glm::mat3(M); // columns are world axes * scale

			// Conservative frustum cull (OBB vs plane SAT, world-space planes)
			bool culled = false;
			for (const Plane& pl : planes)
			{
				const float r =
					std::abs(glm::dot(pl.n, A[0])) * e.x +
					std::abs(glm::dot(pl.n, A[1])) * e.y +
					std::abs(glm::dot(pl.n, A[2])) * e.z;

				const float s = glm::dot(pl.n, cW) + pl.d;

				if (s < -r)
				{
					culled = true;
					break;
				}
			}

			if (culled)
				continue;

			// Survived culling: keep in original order
			result.push_back(it);
		}

		return result;
	}

	std::vector<MaterialRenderInfo> SceneRenderer::FrustumCulledDistanceSortedMaterials(
		const std::vector<MaterialRenderInfo>& _materials,
		const glm::mat4& _view,
		const glm::mat4& _proj,
		const glm::vec3& _posWS,
		DepthSortMode _mode)
	{
		struct Plane
		{
			glm::vec3 n;
			float d; // plane: n·x + d >= 0 => inside
		};

		struct DepthKey
		{
			float  primary;      // zMax or zMin
			float  farthest;     // distance^2 to farthest corner (when cameraInside)
			bool   cameraInside; // true if camera is inside this OBB
			size_t index;        // index into material array
		};

		const glm::mat4 VP = _proj * _view;

		// Extract rows from column-major matrix
		auto getRow = [&](int r)
			{
				return glm::vec4(VP[0][r], VP[1][r], VP[2][r], VP[3][r]);
			};

		const glm::vec4 row0 = getRow(0);
		const glm::vec4 row1 = getRow(1);
		const glm::vec4 row2 = getRow(2);
		const glm::vec4 row3 = getRow(3);

		Plane planes[6] = {
			{ glm::vec3(row3 + row0), (row3 + row0).w }, // Left
			{ glm::vec3(row3 - row0), (row3 - row0).w }, // Right
			{ glm::vec3(row3 + row1), (row3 + row1).w }, // Bottom
			{ glm::vec3(row3 - row1), (row3 - row1).w }, // Top
			{ glm::vec3(row3 + row2), (row3 + row2).w }, // Near
			{ glm::vec3(row3 - row2), (row3 - row2).w }, // Far
		};

		// Normalise planes
		for (Plane& p : planes)
		{
			float invLen = 1.0f / glm::length(p.n);
			p.n *= invLen;
			p.d *= invLen;
		}

		// Helper: is camera inside this OBB (bounds in model space, transform to world)
		auto IsCameraInsideOBB = [&](const glm::mat4& M,
			const glm::vec3& centerMS,
			const glm::vec3& halfExtentsMS) -> bool
			{
				glm::mat4 invM = glm::inverse(M);
				glm::vec3 camMS = glm::vec3(invM * glm::vec4(_posWS, 1.0f));

				glm::vec3 d = camMS - centerMS;
				return std::abs(d.x) <= halfExtentsMS.x &&
					std::abs(d.y) <= halfExtentsMS.y &&
					std::abs(d.z) <= halfExtentsMS.z;
			};

		// Helper: squared distance to farthest corner of OBB from camera
		auto FarthestCornerDistSq = [&](const glm::mat4& M,
			const glm::vec3& centerMS,
			const glm::vec3& halfExtentsMS) -> float
			{
				float maxDist2 = 0.0f;

				for (int sx = -1; sx <= 1; sx += 2)
				{
					for (int sy = -1; sy <= 1; sy += 2)
					{
						for (int sz = -1; sz <= 1; sz += 2)
						{
							glm::vec3 cornerMS = centerMS + glm::vec3(
								float(sx) * halfExtentsMS.x,
								float(sy) * halfExtentsMS.y,
								float(sz) * halfExtentsMS.z
							);
							glm::vec3 cornerWS = glm::vec3(M * glm::vec4(cornerMS, 1.0f));
							float d2 = glm::length(cornerWS - _posWS);
							if (d2 > maxDist2) maxDist2 = d2;
						}
					}
				}
				return maxDist2;
			};

		// Build keys
		std::vector<DepthKey> keys;
		keys.reserve(_materials.size());

		for (size_t i = 0; i < _materials.size(); ++i)
		{
			const auto& it = _materials[i];

			const glm::mat4  M = it.transform;
			const glm::vec3  e = it.materialGroup.boundsHalfExtentsMS;
			const glm::vec3  cMS = it.materialGroup.boundsCenterMS;
			const glm::vec3  cW = glm::vec3(M * glm::vec4(cMS, 1.0f));
			const glm::mat3  A = glm::mat3(M);

			// Conservative frustum cull (OBB vs plane SAT, world-space planes)
			bool culled = false;
			for (const Plane& pl : planes)
			{
				const float r =
					std::abs(glm::dot(pl.n, A[0])) * e.x +
					std::abs(glm::dot(pl.n, A[1])) * e.y +
					std::abs(glm::dot(pl.n, A[2])) * e.z;

				const float s = glm::dot(pl.n, cW) + pl.d;

				if (s < -r)
				{
					culled = true;
					break;
				}
			}

			if (culled) continue;

			// View-space depth extents
			const glm::mat4 VM = _view * M;
			const glm::vec3 centerVS = glm::vec3(VM * glm::vec4(cMS, 1.0f));
			const glm::mat3 AV = glm::mat3(VM);
			const float projZ =
				std::abs(AV[0][2]) * e.x +
				std::abs(AV[1][2]) * e.y +
				std::abs(AV[2][2]) * e.z;

			float primary = 0.0f;
			if (_mode == DepthSortMode::FrontToBack)
			{
				// opaque: zMax, larger first
				const float zMax = centerVS.z + projZ;
				primary = zMax;
			}
			else // BackToFront
			{
				// transparent: zMin, smaller first
				const float zMin = centerVS.z - projZ;
				primary = zMin;
			}

			const bool inside = IsCameraInsideOBB(M, cMS, e);
			const float farDist = inside ? FarthestCornerDistSq(M, cMS, e) : 0.0f;

			DepthKey k;
			k.primary = primary;
			k.farthest = farDist;
			k.cameraInside = inside;
			k.index = i;

			keys.push_back(k);
		}

		// Sort
		std::sort(keys.begin(), keys.end(),
			[_mode](const DepthKey& a, const DepthKey& b)
			{
				const bool frontToBack = (_mode == DepthSortMode::FrontToBack);

				if (a.cameraInside && b.cameraInside)
				{
					if (frontToBack)
					{
						// opaque: both contain camera -> farthest corner closer first
						return a.farthest < b.farthest;
					}
					else
					{
						// transparent: both contain camera -> farthest first (back->front)
						return a.farthest > b.farthest;
					}
				}

				if (a.cameraInside != b.cameraInside)
				{
					if (frontToBack)
					{
						// opaque: draw the ones we are inside first
						return a.cameraInside && !b.cameraInside;
					}
					else
					{
						// transparent: treat "inside" as very close, so put it later
						return !a.cameraInside && b.cameraInside;
					}
				}

				// Neither contains camera: use existing primary depth behaviour
				if (frontToBack)
				{
					// opaque: zMax, larger first
					return a.primary > b.primary;
				}
				else
				{
					// transparent: zMin, smaller first
					return a.primary < b.primary;
				}
			});

		// Build output list
		std::vector<MaterialRenderInfo> result;
		result.reserve(keys.size());
		for (const auto& k : keys)
			result.push_back(_materials[k.index]);

		return result;
	}

}