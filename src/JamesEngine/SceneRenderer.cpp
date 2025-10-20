#include "SceneRenderer.h"

#include "Core.h"

#include "Resources.h"
#include "Camera.h"
#include "Transform.h"
#include "Skybox.h"

namespace JamesEngine
{

	SceneRenderer::SceneRenderer(std::shared_ptr<Core> _core)
		: mCore(_core)
	{
		mDepthShader = mCore.lock()->GetResources()->Load<Shader>("shaders/DepthOnly");
		mObjShader = mCore.lock()->GetResources()->Load<Shader>("shaders/ObjShader");
	}

	void SceneRenderer::RenderScene()
	{
		//ScopedTimer timer("SceneRenderer::RenderScene");

		auto core = mCore.lock();
		auto window = core->GetWindow();
		auto camera = core->GetCamera();

		if (!core->mLightManager->ArePreBakedShadowsUploaded())
		{
			std::vector<std::shared_ptr<Renderer::RenderTexture>> preBakedShadowMaps;
			preBakedShadowMaps.reserve(core->mLightManager->GetPreBakedShadowMaps().size());
			std::vector<glm::mat4> preBakedShadowMatrices;
			preBakedShadowMatrices.reserve(core->mLightManager->GetPreBakedShadowMaps().size());

			for (const PreBakedShadowMap& shadowMap : core->mLightManager->GetPreBakedShadowMaps())
			{
				preBakedShadowMaps.emplace_back(shadowMap.renderTexture);
				preBakedShadowMatrices.emplace_back(shadowMap.lightSpaceMatrix);
			}

			mObjShader->mShader->use();

			mObjShader->mShader->uniform("u_PreBakedShadowMaps", preBakedShadowMaps, 10);
			mObjShader->mShader->uniform("u_PreBakedLightSpaceMatrices", preBakedShadowMatrices);

			mObjShader->mShader->uniform("u_NumPreBaked", 0); // Might temporarily drop suport for pre-baked shadows, may bring back later

			mObjShader->mShader->unuse();

			core->mLightManager->SetPreBakedShadowsUploaded(true);

			std::cout << "Pre-baked shadows uploaded to shader" << std::endl;
		}

		// Global uniforms

		mDepthShader->mShader->use();
		mDepthShader->mShader->uniform("u_AlphaCutoff", mAlphaCutoff);
		mDepthShader->mShader->unuse();

		mObjShader->mShader->use();
		mObjShader->mShader->uniform("u_Projection", camera->GetProjectionMatrix());
		mObjShader->mShader->uniform("u_View", camera->GetViewMatrix());
		mObjShader->mShader->uniform("u_ViewPos", camera->GetPosition());
		mObjShader->mShader->uniform("u_Ambient", core->mLightManager->GetAmbient());
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
			int winW, winH;
			window->GetWindowSize(winW, winH);
			const float aspect = (winH > 0.0f) ? (winW / winH) : 16.0f / 9.0f;
			const float camNear = camera->GetNearClip();
			const float camFar = camera->GetFarClip();

			// Shadow configuration
			const float shadowDistance = 250;
			const float lambda = 0.6f; // 0..1 (e.g. 0.6)
			const glm::vec3 lightDir = glm::normalize(core->mLightManager->GetDirectionalLightDirection());

			auto& cascades = core->mLightManager->GetShadowCascades();
			const int numCascades = (int)cascades.size();

			// Build practical split depths over [camNear, min(shadowDistance, camFar)]
			const float maxShadowZ = std::min(shadowDistance, camFar);
			std::vector<float> splitFar(numCascades);
			for (int i = 0; i < numCascades; ++i)
			{
				float t = float(i + 1) / float(numCascades);
				float lin = camNear + (maxShadowZ - camNear) * t;
				float log = camNear * std::pow(maxShadowZ / camNear, t);
				// Replace the glm::lerp call with the following implementation
				splitFar[i] = lin + lambda * (log - lin);
			}

			// Helper to build frustum-slice corners in world space (8 points)
			auto buildSliceCornersWS = [&](float zn, float zf)
				{
					std::array<glm::vec3, 8> c;
					const float nh = 2.0f * std::tan(vfov * 0.5f) * zn;
					const float nw = nh * aspect;
					const float fh = 2.0f * std::tan(vfov * 0.5f) * zf;
					const float fw = fh * aspect;

					const glm::vec3 nc = camPos + camFwd * zn;
					const glm::vec3 fc = camPos + camFwd * zf;

					// near (t/l, t/r, b/l, b/r)
					c[0] = nc + (camUp * (nh * 0.5f) - camRight * (nw * 0.5f));
					c[1] = nc + (camUp * (nh * 0.5f) + camRight * (nw * 0.5f));
					c[2] = nc + (-camUp * (nh * 0.5f) - camRight * (nw * 0.5f));
					c[3] = nc + (-camUp * (nh * 0.5f) + camRight * (nw * 0.5f));
					// far
					c[4] = fc + (camUp * (fh * 0.5f) - camRight * (fw * 0.5f));
					c[5] = fc + (camUp * (fh * 0.5f) + camRight * (fw * 0.5f));
					c[6] = fc + (-camUp * (fh * 0.5f) - camRight * (fw * 0.5f));
					c[7] = fc + (-camUp * (fh * 0.5f) + camRight * (fw * 0.5f));
					return c;
				};

			// Per-cascade margins (world units)
			const float pcfMarginXY = 5.f;
			const float casterMarginZ = 50.0f;

			float prevFar = camNear;

			mDepthShader->mShader->use();

			for (int ci = 0; ci < numCascades; ++ci)
			{
				ShadowCascade& cascade = cascades[ci];

				const float zn = prevFar;
				const float zf = splitFar[ci];
				prevFar = zf;

				// 1) Frustum slice corners in world space
				auto cornersWS = buildSliceCornersWS(zn, zf);

				// Compute slice center (average of corners)
				glm::vec3 center(0.0f);
				for (auto& p : cornersWS) center += p;
				center *= (1.0f / 8.0f);

				// 2) Light view (look from far along -lightDir toward center)
				const float eyeDist = 1000.0f; // any large number covering scene depth
				glm::mat4 lightView = glm::lookAt(center - lightDir * eyeDist, center, glm::vec3(0, 1, 0));

				// 3) Transform corners to light space and fit a tight AABB
				glm::vec3 minLS(+FLT_MAX), maxLS(-FLT_MAX);
				for (auto& p : cornersWS)
				{
					glm::vec4 q = lightView * glm::vec4(p, 1.0f);
					minLS = glm::min(minLS, glm::vec3(q));
					maxLS = glm::max(maxLS, glm::vec3(q));
				}

				// 4) Pad for filtering and nearby casters
				minLS.x -= pcfMarginXY;  maxLS.x += pcfMarginXY;
				minLS.y -= pcfMarginXY;  maxLS.y += pcfMarginXY;
				minLS.z -= casterMarginZ;     // extend toward the light
				maxLS.z += casterMarginZ;     // extend away from the light

				// 5) Ortho projection from this AABB (OpenGL NDC Z = [-1,1])
				glm::mat4 lightProj = glm::ortho(minLS.x, maxLS.x,
					minLS.y, maxLS.y,
					-maxLS.z, -minLS.z);

				// 6) Final matrix for the shader
				cascade.lightSpaceMatrix = lightProj * lightView;

				mDepthShader->mShader->uniform("u_LightSpaceMatrix", cascade.lightSpaceMatrix);

				// --- Render this cascade ---
				cascade.renderTexture->clear();
				cascade.renderTexture->bind();
				glViewport(0, 0, cascade.renderTexture->getWidth(), cascade.renderTexture->getHeight());

				for (const auto& submission : mSubmissions)
				{
					if (submission.shadowMode == ShadowMode::None)
						continue;

					std::shared_ptr<Model> modelToRender = submission.model;
					// If using proxy shadow model, switch to that
					if (submission.shadowMode == ShadowMode::Proxy)
						modelToRender = submission.shadowModel;

					mDepthShader->mShader->uniform("u_Model", submission.transform);

					// Avoids copying or double-deleting the underlying GL object.
					auto asShared = [](const Renderer::Texture& t) -> std::shared_ptr<Renderer::Texture> {
						return std::shared_ptr<Renderer::Texture>(const_cast<Renderer::Texture*>(&t), [](Renderer::Texture*) {}); // no-op deleter
						};

					// Render each material, will change so that we don't do it blindly e.g. only when in view of camera
					for (const auto& materialGroup : modelToRender->mModel->GetMaterialGroups())
					{
						const auto& pbr = materialGroup.pbr;
						const auto& embedded = modelToRender->mModel->GetEmbeddedTextures();

						// Culling per material (same as before)
						GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);
						if (pbr.doubleSided) glDisable(GL_CULL_FACE);
						else glEnable(GL_CULL_FACE);

						// Albedo (unit 0)
						if (pbr.baseColorTexIndex >= 0 && pbr.baseColorTexIndex < (int)embedded.size())
						{
							mDepthShader->mShader->uniform("u_AlbedoMap", asShared(embedded[pbr.baseColorTexIndex]), 0);
							mDepthShader->mShader->uniform("u_HasAlbedoMap", 1);
						}
						else
						{
							mDepthShader->mShader->uniform("u_HasAlbedoMap", 0);
						}

						// Normal (unit 1)
						if (pbr.normalTexIndex >= 0 && pbr.normalTexIndex < (int)embedded.size())
						{
							mDepthShader->mShader->uniform("u_NormalMap", asShared(embedded[pbr.normalTexIndex]), 1);
							mDepthShader->mShader->uniform("u_HasNormalMap", 1);
						}
						else
						{
							mDepthShader->mShader->uniform("u_HasNormalMap", 0);
						}

						// Metallic-Roughness (unit 2)
						if (pbr.metallicRoughnessTexIndex >= 0 && pbr.metallicRoughnessTexIndex < (int)embedded.size())
						{
							mDepthShader->mShader->uniform("u_MetallicRoughnessMap", asShared(embedded[pbr.metallicRoughnessTexIndex]), 2);
							mDepthShader->mShader->uniform("u_HasMetallicRoughnessMap", 1);
						}
						else
						{
							mDepthShader->mShader->uniform("u_HasMetallicRoughnessMap", 0);
						}

						// Occlusion (unit 3)
						if (pbr.occlusionTexIndex >= 0 && pbr.occlusionTexIndex < (int)embedded.size())
						{
							mDepthShader->mShader->uniform("u_OcclusionMap", asShared(embedded[pbr.occlusionTexIndex]), 3);
							mDepthShader->mShader->uniform("u_HasOcclusionMap", 1);
						}
						else
						{
							mDepthShader->mShader->uniform("u_HasOcclusionMap", 0);
						}

						// Emissive (unit 4)
						if (pbr.emissiveTexIndex >= 0 && pbr.emissiveTexIndex < (int)embedded.size())
						{
							mDepthShader->mShader->uniform("u_EmissiveMap", asShared(embedded[pbr.emissiveTexIndex]), 4);
							mDepthShader->mShader->uniform("u_HasEmissiveMap", 1);
						}
						else
						{
							mDepthShader->mShader->uniform("u_HasEmissiveMap", 0);
						}

						// Transmission (unit 5)
						if (pbr.transmissionTexIndex >= 0 && pbr.transmissionTexIndex < (int)embedded.size())
						{
							mDepthShader->mShader->uniform("u_TransmissionTex", asShared(embedded[pbr.transmissionTexIndex]), 5);
							mDepthShader->mShader->uniform("u_HasTransmissionTex", 1);
						}
						else
						{
							mDepthShader->mShader->uniform("u_HasTransmissionTex", 0);
						}

						// Material factors
						mDepthShader->mShader->uniform("u_BaseColorFactor", pbr.baseColorFactor);
						mDepthShader->mShader->uniform("u_MetallicFactor", pbr.metallicFactor);
						mDepthShader->mShader->uniform("u_RoughnessFactor", pbr.roughnessFactor);
						mDepthShader->mShader->uniform("u_NormalScale", pbr.normalScale);
						mDepthShader->mShader->uniform("u_OcclusionStrength", pbr.occlusionStrength);
						mDepthShader->mShader->uniform("u_EmissiveFactor", pbr.emissiveFactor);
						mDepthShader->mShader->uniform("u_TransmissionFactor", pbr.transmissionFactor);
						mDepthShader->mShader->uniform("u_IOR", pbr.ior);

						// Fallbacks
						mDepthShader->mShader->uniform("u_AlbedoFallback", mBaseColorStrength);
						mDepthShader->mShader->uniform("u_MetallicFallback", mMetallicness);
						mDepthShader->mShader->uniform("u_RoughnessFallback", mRoughness);
						mDepthShader->mShader->uniform("u_AOFallback", mAOStrength);
						mDepthShader->mShader->uniform("u_EmissiveFallback", mEmmisive);

						// Alpha
						int alphaMode = 0;
						if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaMask)  alphaMode = 1;
						if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaBlend) alphaMode = 2;
						mDepthShader->mShader->uniform("u_AlphaMode", alphaMode);
						mDepthShader->mShader->uniform("u_AlphaCutoff", pbr.alphaCutoff);

						// Draw this material only
						const GLsizei vertCount = static_cast<GLsizei>(materialGroup.faces.size() * 3);
						mDepthShader->mShader->draw(materialGroup.vao, vertCount);

						// Restore culling to previous state
						if (prevCullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
					}
				}

				cascade.renderTexture->unbind();
			}

			mDepthShader->mShader->unuse();

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

		// Normal rendering of the scene
		core->mSkybox->RenderSkybox();

		mObjShader->mShader->use();

		for (const auto& submission : mSubmissions)
		{
			std::shared_ptr<Model> modelToRender = submission.model;

			mObjShader->mShader->uniform("u_Model", submission.transform);

			// This avoids copying or double-deleting the underlying GL object.
			auto asShared = [](const Renderer::Texture& t) -> std::shared_ptr<Renderer::Texture> {
				return std::shared_ptr<Renderer::Texture>(const_cast<Renderer::Texture*>(&t), [](Renderer::Texture*) {}); // no-op deleter
				};

			for (const auto& materialGroup : modelToRender->mModel->GetMaterialGroups())
			{
				const auto& pbr = materialGroup.pbr;
				const auto& embedded = modelToRender->mModel->GetEmbeddedTextures();

				// Culling per material (same as before)
				GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);
				if (pbr.doubleSided) glDisable(GL_CULL_FACE);
				else glEnable(GL_CULL_FACE);

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

				// Fallbacks
				mObjShader->mShader->uniform("u_AlbedoFallback", mBaseColorStrength);
				mObjShader->mShader->uniform("u_MetallicFallback", mMetallicness);
				mObjShader->mShader->uniform("u_RoughnessFallback", mRoughness);
				mObjShader->mShader->uniform("u_AOFallback", mAOStrength);
				mObjShader->mShader->uniform("u_EmissiveFallback", mEmmisive);

				// Alpha
				int alphaMode = 0;
				if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaMask)  alphaMode = 1;
				if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaBlend) alphaMode = 2;
				mObjShader->mShader->uniform("u_AlphaMode", alphaMode);
				mObjShader->mShader->uniform("u_AlphaCutoff", pbr.alphaCutoff);

				// Draw this material only
				const GLsizei vertCount = static_cast<GLsizei>(materialGroup.faces.size() * 3);
				mObjShader->mShader->draw(materialGroup.vao, vertCount);

				// Restore culling to previous state
				if (prevCullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
			}
		}

		mObjShader->mShader->unuse();

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
	}

	void SceneRenderer::ClearScene()
	{
		mSubmissions.clear();
	}

}