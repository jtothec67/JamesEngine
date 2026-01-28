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
		mScalarBlurShader = mCore.lock()->GetResources()->Load<Shader>("shaders/ScalarBlurShader");
		mVec3BlurShader = mCore.lock()->GetResources()->Load<Shader>("shaders/Vec3BlurShader");
		mBrightPassShader = mCore.lock()->GetResources()->Load<Shader>("shaders/BrightPass");
		mDownsample2x = mCore.lock()->GetResources()->Load<Shader>("shaders/Downsample2x");
		mUpsampleAdd = mCore.lock()->GetResources()->Load<Shader>("shaders/BloomUpsampleAdd");
		mCompositeShader = mCore.lock()->GetResources()->Load<Shader>("shaders/CompositeShader");
		mToneMapShader = mCore.lock()->GetResources()->Load<Shader>("shaders/ToneMap");
		mOcclusionBoxShader = mCore.lock()->GetResources()->Load<Shader>("shaders/OcclusionBoxShader");

		// Size of 1,1 just to initialize
		mShadingPass = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::ColourAndDepth);
		mDepthPass = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::Depth);
		mAORaw = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::PostProcessTarget);
		mAOIntermediate = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::PostProcessTarget);
		mAOBlurred = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::PostProcessTarget);
		mBrightPassScene = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::Colour);
		mBloomIntermediate = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::Colour);
		mBloom = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::Colour);
		mCompositeScene = std::make_shared<Renderer::RenderTexture>(1, 1, Renderer::RenderTextureType::Colour);

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

		// Helper lambda to add a triangle
		auto AddCubeFace = [&](const glm::vec3& a,
			const glm::vec3& b,
			const glm::vec3& c)
			{
				Renderer::Face face;
				face.a.m_position = a;
				face.b.m_position = b;
				face.c.m_position = c;

				// Texcoords not used for occlusion
				face.a.m_texcoords = glm::vec2(0.0f);
				face.b.m_texcoords = glm::vec2(0.0f);
				face.c.m_texcoords = glm::vec2(0.0f);

				mCube->add(face);
			};

		glm::vec3 p000(-1.0f, -1.0f, -1.0f);
		glm::vec3 p001(-1.0f, -1.0f, 1.0f);
		glm::vec3 p010(-1.0f, 1.0f, -1.0f);
		glm::vec3 p011(-1.0f, 1.0f, 1.0f);
		glm::vec3 p100(1.0f, -1.0f, -1.0f);
		glm::vec3 p101(1.0f, -1.0f, 1.0f);
		glm::vec3 p110(1.0f, 1.0f, -1.0f);
		glm::vec3 p111(1.0f, 1.0f, 1.0f);

		// -X face
		AddCubeFace(p000, p010, p011);
		AddCubeFace(p000, p011, p001);

		// +X face
		AddCubeFace(p100, p101, p111);
		AddCubeFace(p100, p111, p110);

		// -Y face
		AddCubeFace(p000, p001, p101);
		AddCubeFace(p000, p101, p100);

		// +Y face
		AddCubeFace(p010, p110, p111);
		AddCubeFace(p010, p111, p011);

		// -Z face
		AddCubeFace(p000, p100, p110);
		AddCubeFace(p000, p110, p010);

		// +Z face
		AddCubeFace(p001, p011, p111);
		AddCubeFace(p001, p111, p101);


		// Set initial default parameters
		EnableSSAO(true);
		SetSSAORadius(0.2f);
		SetSSAOBias(0.06f);
		SetSSAOPower(1.4f);
		SetSSAOBlurScale(1.0f);

		SetAOStrength(1.0f);
		SetAOSpecularScale(1.0f);
		SetAOMin(0.05f);

		EnableBloom(true);
		SetBloomThreshold(1.3f);
		SetBloomStrength(1.f);
		SetBloomKnee(0.65f);
		SetBloomLevels(5);
		SetBloomMinSize(16);

		SetSoftShadowBase(1.f);
		SetSoftShadowScale(10.f);
		SetShadowBiasSlope(0.0022f);
		SetShadowBiasMin(0.0020f);
		SetShadowNormalOffsetScale(2.f);

		SetExposure(1.0f);
	}

	void SceneRenderer::SetBloomLevels(int _levels) // Annoying
	{
		mBloomLevels = _levels;
		mBloomDown.clear();
		mBloomTemp.clear();
		mBloomBlur.clear();
		mBloomDown.resize(mBloomLevels);
		mBloomTemp.resize(mBloomLevels);
		mBloomBlur.resize(mBloomLevels);
		int winW, winH;
		mCore.lock()->GetWindow()->GetWindowSize(winW, winH);
		int w = winW;
		int h = winH;
		for (int i = 0; i < mBloomLevels; ++i)
		{
			mBloomDown[i] = std::make_shared<Renderer::RenderTexture>(w, h, Renderer::RenderTextureType::Colour);
			mBloomTemp[i] = std::make_shared<Renderer::RenderTexture>(w, h, Renderer::RenderTextureType::Colour);
			mBloomBlur[i] = std::make_shared<Renderer::RenderTexture>(w, h, Renderer::RenderTextureType::Colour);

			if (w <= mBloomMinSize || h <= mBloomMinSize)
				break;
		}
	}

	void SceneRenderer::RenderScene()
	{
		//ScopedTimer timer("SceneRenderer::RenderScene");

		mFrameIndex++;

		const int writeQueryIndex = int(mFrameIndex & 1);
		const int readQueryIndex = int((mFrameIndex + 1) & 1);

		// Update visible/hasResult from the previous frame's queries (non-stalling)
		for (auto& pair : mOcclusionCache)
		{
			OcclusionInfo& occlusionInfo = pair.second;

			GLuint available = 0;
			glGetQueryObjectuiv(occlusionInfo.queryIds[readQueryIndex], GL_QUERY_RESULT_AVAILABLE, &available);
			if (!available)
				continue; // do not stall; keep previous state

			GLuint anySamplesPassed = 0;
			glGetQueryObjectuiv(occlusionInfo.queryIds[readQueryIndex], GL_QUERY_RESULT, &anySamplesPassed);

			occlusionInfo.visible = (anySamplesPassed != 0);
			occlusionInfo.hasResult = true;
		}

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
			mBrightPassScene->resize(winW, winH);
			int w = winW;
			int h = winH;
			for (int i = 0; i < mBloomLevels; ++i)
			{
				w = std::max(w / 2, 1);
				h = std::max(h / 2, 1);
				mBloomDown[i]->resize(w, h);
				mBloomTemp[i]->resize(w, h);
				mBloomBlur[i]->resize(w, h);

				if (w <= mBloomMinSize || h <= mBloomMinSize)
					break;
			}
			mBloomIntermediate->resize(winW, winH);
			mBloom->resize(winW, winH);
			mCompositeScene->resize(winW, winH);

			mLastViewportSize = glm::ivec2(winW, winH);
		}

		// Camera info
		const glm::mat4 camView = camera->GetViewMatrix();
		const glm::mat4 camProj = camera->GetProjectionMatrix();
		const glm::vec3 camPos = camera->GetPosition();
		const glm::mat4 VP = camProj * camView;
		const glm::vec3 camFwd = -camera->GetEntity()->GetComponent<Transform>()->GetForward();
		const glm::vec3 camUp = camera->GetEntity()->GetComponent<Transform>()->GetUp();
		const glm::vec3 camRight = camera->GetEntity()->GetComponent<Transform>()->GetRight();
		const float vfov = glm::radians(camera->GetFov());
		const float aspect = (winH > 0) ? (static_cast<float>(winW) / static_cast<float>(winH)) : (16.0f / 9.0f);
		const float camNear = camera->GetNearClip();
		const float camFar = camera->GetFarClip();

		// Global uniforms
		mObjShader->mShader->use();
		mObjShader->mShader->uniform("u_Projection", camProj);
		mObjShader->mShader->uniform("u_View", camView);
		mObjShader->mShader->uniform("u_ViewPos", camPos);

		if (!core->mLightManager->GetShadowCascades().empty())
		{
			// --- Render-state for depth-only shadow pass ---
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDisable(GL_MULTISAMPLE);
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

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

				auto inv = glm::inverse(cascadeProj * camView);

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

				float worldWidth = maxX - minX;
				float resX = (float)cascade.resolution.x;
				cascade.worldUnitsPerTexel = worldWidth / resX;

				mDepthShader->mShader->use();
				mDepthShader->mShader->uniform("u_View", lightView);
				mDepthShader->mShader->uniform("u_Projection", lightProj);

				mDepthAlphaShader->mShader->use();
				mDepthAlphaShader->mShader->uniform("u_View", lightView);
				mDepthAlphaShader->mShader->uniform("u_Projection", lightProj);

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
					}
					else
					{
						mDepthShader->mShader->use();

						mDepthShader->mShader->uniform("u_Model", shadowMaterial.transform);

						const GLsizei vertCount = static_cast<GLsizei>(materialGroup.faces.size() * 3);
						mDepthShader->mShader->draw(materialGroup.vao, vertCount);
					}

					// Restore culling
					if (prevCullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
				}
			}

			window->ResetGLModes();

			// Want to upload shadow maps at the start, but it needs to be done after the shadow maps are rendered
			std::vector<std::shared_ptr<Renderer::RenderTexture>> shadowMaps;
			shadowMaps.reserve(core->mLightManager->GetShadowCascades().size());
			std::vector<glm::mat4> shadowMatrices;
			shadowMatrices.reserve(core->mLightManager->GetShadowCascades().size());
			std::vector<float> cascadeTexelScale;
			cascadeTexelScale.reserve(core->mLightManager->GetShadowCascades().size());
			std::vector<float> cascadeWorldTexelSize;
			cascadeWorldTexelSize.reserve(cascades.size());

			float refWorldPerTexel = (cascades[0].worldUnitsPerTexel > 0.0f) ? cascades[0].worldUnitsPerTexel : 1.0f;

			for (const ShadowCascade& cascade : core->mLightManager->GetShadowCascades())
			{
				shadowMaps.emplace_back(cascade.renderTexture);
				shadowMatrices.emplace_back(cascade.lightSpaceMatrix);

				float worldPerTexel = cascade.worldUnitsPerTexel;
				float scale = 1.0f;

				if (worldPerTexel > 0.0f)
				{
					// How many cascade-i texels equal one "reference texel" in world space?
					scale = refWorldPerTexel / worldPerTexel;
				}

				cascadeTexelScale.push_back(scale);
				cascadeWorldTexelSize.push_back(worldPerTexel);
			}

			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_NumCascades", (int)shadowMaps.size());
			mObjShader->mShader->uniform("u_ShadowMaps", shadowMaps, 20);
			mObjShader->mShader->uniform("u_LightSpaceMatrices", shadowMatrices);
			mObjShader->mShader->uniform("u_CascadeTexelScale", cascadeTexelScale);
			mObjShader->mShader->uniform("u_CascadeWorldTexelSize", cascadeWorldTexelSize);
		}
		else
		{
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_NumCascades", 0);
		}

		// Prepare window for rendering
		window->Update();
		window->ClearWindow();

		window->ResetGLModes();


		std::vector<MaterialRenderInfo> frustumCulledOpaqueMaterials = FrustumCulledMaterials(
			mOpaqueMaterials,
			camView,
			camProj);

		std::vector<MaterialRenderInfo> frustumCulledTransparentMaterials = FrustumCulledMaterials(
			mTransparentMaterials,
			camView,
			camProj);

		// This avoids copying or double-deleting the underlying GL object.
		auto asShared = [](const Renderer::Texture& t) -> std::shared_ptr<Renderer::Texture> {
			return std::shared_ptr<Renderer::Texture>(const_cast<Renderer::Texture*>(&t), [](Renderer::Texture*) {}); // no-op deleter
			};

		// DEPTH PASS
		// Draw to only the depth buffer of the shading pass (maybe change name)
		mShadingPass->clear();
		mShadingPass->bind();
		glViewport(0, 0, mShadingPass->getWidth(), mShadingPass->getHeight());

		// Depth-only state
		glDisable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDisable(GL_MULTISAMPLE);
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

		mDepthShader->mShader->use();
		mDepthShader->mShader->uniform("u_View", camView);
		mDepthShader->mShader->uniform("u_Projection", camProj);

		mDepthAlphaShader->mShader->use();
		mDepthAlphaShader->mShader->uniform("u_View", camView);
		mDepthAlphaShader->mShader->uniform("u_Projection", camProj);

		// OPAQUES
		for (const auto& opaqueMaterial : frustumCulledOpaqueMaterials)
		{
			auto occlusionIterator = mOcclusionCache.find(opaqueMaterial.occlusionKey);
			if (occlusionIterator != mOcclusionCache.end())
			{
				const OcclusionInfo& occlusionInfo = occlusionIterator->second;

				// Skip writing depth only if we are confident it is occluded
				if (occlusionInfo.hasResult && !occlusionInfo.visible)
					continue;
			}

			const auto& pbr = opaqueMaterial.materialGroup.pbr;
			GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);
			if (pbr.doubleSided) glDisable(GL_CULL_FACE);
			else glEnable(GL_CULL_FACE);

			if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaOpaque)
			{
				mDepthShader->mShader->use();
				mDepthShader->mShader->uniform("u_Model", opaqueMaterial.transform);

				const GLsizei vertCount = static_cast<GLsizei>(opaqueMaterial.materialGroup.faces.size() * 3);
				mDepthShader->mShader->draw(opaqueMaterial.materialGroup.vao, vertCount);
			}
			else
			{
				mDepthAlphaShader->mShader->use();
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
			}

			if (prevCullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		}

		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		for (const auto& transparentMaterial : frustumCulledTransparentMaterials)
		{
			const auto& pbr = transparentMaterial.materialGroup.pbr;

			GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);
			if (pbr.doubleSided) glDisable(GL_CULL_FACE);
			else glEnable(GL_CULL_FACE);

			mDepthAlphaShader->mShader->use();
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

			if (prevCullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		}

		glDisable(GL_BLEND);

		glDepthMask(GL_FALSE); // Disable depth writes for occlusion testing
		glDisable(GL_CULL_FACE);

		// Issue occlusion queries using the depth we just wrote
		mOcclusionBoxShader->mShader->use();
		mOcclusionBoxShader->mShader->uniform("u_View", camView);
		mOcclusionBoxShader->mShader->uniform("u_Projection", camProj);

		for (const MaterialRenderInfo& material : frustumCulledOpaqueMaterials)
		{
			auto cacheIterator = mOcclusionCache.find(material.occlusionKey);
			OcclusionInfo& occlusionInfo = cacheIterator->second;

			if (mFrameIndex - occlusionInfo.lastFrameTested < 5) // Number must be odd, to do with double-buffered queries
				continue; // Recently tested

			if (cacheIterator == mOcclusionCache.end())
				continue; // Should not happen

			// Set transforms + bounds (model-space)
			mOcclusionBoxShader->mShader->uniform("u_Model", material.transform);
			mOcclusionBoxShader->mShader->uniform("u_BoundsCenterMS", material.materialGroup.boundsCenterMS);
			mOcclusionBoxShader->mShader->uniform("u_BoundsHalfExtentsMS", material.materialGroup.boundsHalfExtentsMS);

			glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, occlusionInfo.queryIds[writeQueryIndex]);
			mOcclusionBoxShader->mShader->draw(mCube.get());
			glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);
			occlusionInfo.hasResult = false;
			occlusionInfo.lastFrameTested = mFrameIndex;
		}

		glEnable(GL_CULL_FACE);

		// Restore color writes
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		if (mSSAOEnabled)
		{
			// SSAO PASS
			// Raw AO
			mAORaw->clear();
			mAORaw->bind();
			mSSAOShader->mShader->use();
			mSSAOShader->mShader->uniform("u_Depth", mShadingPass->getDepthTextureId());
			mSSAOShader->mShader->uniform("u_Proj", camProj);
			mSSAOShader->mShader->uniform("u_InvView", glm::inverse(camView));
			mSSAOShader->mShader->uniform("u_InvProj", glm::inverse(camProj));
			mSSAOShader->mShader->uniform("u_InvResolution", glm::vec2(1.0f / mShadingPass->getWidth(), 1.0f / mShadingPass->getHeight()));
			mSSAOShader->mShader->draw(mRect.get());

			// Blur AO - horizontal
			mAOIntermediate->clear();
			mAOIntermediate->bind();
			mScalarBlurShader->mShader->use();
			mScalarBlurShader->mShader->uniform("u_RawAO", mAORaw);
			mScalarBlurShader->mShader->uniform("u_InvResolution", glm::vec2(1.0f / mAORaw->getWidth(), 1.0f / mAORaw->getHeight()));
			mScalarBlurShader->mShader->uniform("u_Direction", glm::vec2(1, 0));
			mScalarBlurShader->mShader->uniform("u_StepScale", mSSAOBlurScale);
			mScalarBlurShader->mShader->draw(mRect.get());

			// Blur AO - vertical
			mAOBlurred->clear();
			mAOBlurred->bind();
			mScalarBlurShader->mShader->use();
			mScalarBlurShader->mShader->uniform("u_RawAO", mAOIntermediate);
			mScalarBlurShader->mShader->uniform("u_Direction", glm::vec2(0, 1));
			mScalarBlurShader->mShader->draw(mRect.get());
		}


		mShadingPass->bind();
		glViewport(0, 0, mShadingPass->getWidth(), mShadingPass->getHeight());

		glDisable(GL_DEPTH_TEST);

		core->mSkybox->RenderSkybox();

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_EQUAL);
		glDepthMask(GL_FALSE);
		glDisable(GL_BLEND);

		mObjShader->mShader->use();
		if (mSSAOEnabled)
		{
			mObjShader->mShader->uniform("u_SSAO", mAOBlurred, 27);
			mObjShader->mShader->uniform("u_InvColorResolution", glm::vec2(1.f / mShadingPass->getWidth(), 1.f / mShadingPass->getHeight()));
		}

		// Fallbacks
		mObjShader->mShader->uniform("u_AlbedoFallback", mBaseColorStrength);
		mObjShader->mShader->uniform("u_MetallicFallback", mMetallicness);
		mObjShader->mShader->uniform("u_RoughnessFallback", mRoughness);
		mObjShader->mShader->uniform("u_AOFallback", mAOFallback);
		mObjShader->mShader->uniform("u_EmissiveFallback", mEmmisive);

		int materialCount = 0;
		// SHADING PASS
		for (const auto& opaqueMaterial : frustumCulledOpaqueMaterials)
		{
			auto cacheIterator = mOcclusionCache.find(opaqueMaterial.occlusionKey);
			if (cacheIterator != mOcclusionCache.end())
			{
				const OcclusionInfo& occlusionInfo = cacheIterator->second;

				// If we have a valid previous result and it was occluded, skip drawing
				if (occlusionInfo.hasResult && !occlusionInfo.visible)
					continue;
			}

			materialCount++;

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
		for (const auto& transparentMaterial : frustumCulledTransparentMaterials)
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

		// States for post-process
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_BLEND);

		if (mBloomEnabled)
		{
			// Bright pass, determine what contributes to bloom
			mBrightPassScene->clear();
			mBrightPassScene->bind();
			glViewport(0, 0, mBrightPassScene->getWidth(), mBrightPassScene->getHeight());
			mBrightPassShader->mShader->use();
			mBrightPassShader->mShader->uniform("u_Scene", mShadingPass, 29);
			mBrightPassShader->mShader->draw(mRect.get());

			// Downsample and blur bright pass
			int usedLevels = 0;
			std::shared_ptr<Renderer::RenderTexture> src = mBrightPassScene;
			for (int i = 0; i < mBloomLevels; ++i)
			{
				auto& down = mBloomDown[i];
				auto& temp = mBloomTemp[i];
				auto& blur = mBloomBlur[i];

				// Downsample src -> down
				down->clear();
				down->bind();
				glViewport(0, 0, down->getWidth(), down->getHeight());
				mDownsample2x->mShader->use();
				mDownsample2x->mShader->uniform("u_RawTexture", src, 29);
				mDownsample2x->mShader->draw(mRect.get());

				// Horizontal
				temp->clear();
				temp->bind();
				glViewport(0, 0, temp->getWidth(), temp->getHeight());
				mVec3BlurShader->mShader->use();
				mVec3BlurShader->mShader->uniform("u_Source", down);
				mVec3BlurShader->mShader->uniform("u_InvResolution", glm::vec2(1.0f / down->getWidth(), 1.0f / down->getHeight()));
				mVec3BlurShader->mShader->uniform("u_Direction", glm::vec2(1, 0));
				mVec3BlurShader->mShader->draw(mRect.get());

				// Vertical
				blur->clear();
				blur->bind();
				glViewport(0, 0, blur->getWidth(), blur->getHeight());
				mVec3BlurShader->mShader->use();
				mVec3BlurShader->mShader->uniform("u_Source", temp);
				mVec3BlurShader->mShader->uniform("u_Direction", glm::vec2(0, 1));
				mVec3BlurShader->mShader->draw(mRect.get());

				src = blur;
				usedLevels = i + 1;

				if (down->getWidth() <= mBloomMinSize || down->getHeight() <= mBloomMinSize)
					break;
			}

			// Combine pyramid (smallest -> largest) into mBloom (ends up at half-res)
			if (usedLevels > 0)
			{
				int last = usedLevels - 1;

				// Start accumulator as the smallest blurred level
				std::shared_ptr<Renderer::RenderTexture> accum = mBloomBlur[last];

				// Ping-pong between these two outputs
				bool writeToBloom = true;

				for (int i = last - 1; i >= 0; --i)
				{
					auto& high = mBloomBlur[i];

					// Choose output RT and resize to this level’s size
					std::shared_ptr<Renderer::RenderTexture> outRT = writeToBloom ? mBloom : mBloomIntermediate;
					outRT->resize(high->getWidth(), high->getHeight());

					outRT->clear();
					outRT->bind();
					glViewport(0, 0, outRT->getWidth(), outRT->getHeight());
					mUpsampleAdd->mShader->use();
					mUpsampleAdd->mShader->uniform("u_LowRes", accum, 25);
					mUpsampleAdd->mShader->uniform("u_HighRes", high, 24);
					mUpsampleAdd->mShader->uniform("u_LowStrength", 1.0f);
					mUpsampleAdd->mShader->draw(mRect.get());

					accum = outRT;
					writeToBloom = !writeToBloom;
				}

				// Ensure final result is in mBloom (swap if needed)
				if (accum != mBloom)
				{
					// One last copy pass using the same shader
					// (cheapest way without a dedicated copy shader)
					mBloom->resize(accum->getWidth(), accum->getHeight());
					mBloom->clear();
					mBloom->bind();
					glViewport(0, 0, mBloom->getWidth(), mBloom->getHeight());
					mUpsampleAdd->mShader->use();
					mUpsampleAdd->mShader->uniform("u_LowRes", accum, 25);
					mUpsampleAdd->mShader->uniform("u_HighRes", accum, 24);
					mUpsampleAdd->mShader->uniform("u_LowStrength", 0.0f); // Out = high
					mUpsampleAdd->mShader->draw(mRect.get());
				}
			}
			else
			{
				// If bloom levels somehow ended up 0, just clear bloom
				mBloom->clear();
			}
		}


		// COMBINE PASS
		mCompositeScene->clear();
		mCompositeScene->bind();
		glViewport(0, 0, mCompositeScene->getWidth(), mCompositeScene->getHeight());
		mCompositeShader->mShader->use();
		mCompositeShader->mShader->uniform("u_Scene", mShadingPass, 29);
		if (mBloomEnabled)
		{
			mCompositeShader->mShader->uniform("u_Bloom", mBloom, 26); // Should find a way to not eyeball the texture unit
		}
		mCompositeShader->mShader->draw(mRect.get());
		mCompositeScene->unbind();

		glViewport(0, 0, winW, winH);
		mToneMapShader->mShader->use();
		mToneMapShader->mShader->uniform("u_HDRScene", mCompositeScene, 29);
		mToneMapShader->mShader->draw(mRect.get());

		window->ResetGLModes();

		ClearScene();
	}

	void SceneRenderer::AddModel(int _entityId, std::shared_ptr<Model> _model, const glm::mat4& _transform, const ShadowOverride& _shadow)
	{
		auto opaque = Renderer::Model::PBRMaterial::AlphaMode::AlphaOpaque;
		auto mask = Renderer::Model::PBRMaterial::AlphaMode::AlphaMask;
		auto blend = Renderer::Model::PBRMaterial::AlphaMode::AlphaBlend;

		uint32_t materialGroupIndex = 0; // For occlusion hash
		for (const auto& materialGroup : _model->mModel->GetMaterialGroups())
		{
			materialGroupIndex++;

			uint64_t occlusionKey = (uint64_t(_entityId) << 32) | uint64_t(materialGroupIndex);

			const auto& pbr = materialGroup.pbr;

			if (pbr.alphaMode == opaque || pbr.alphaMode == mask)
			{
				mOpaqueMaterials.push_back({ const_cast<Renderer::Model::MaterialGroup&>(materialGroup), _model, _transform, occlusionKey });
			}
			else if (pbr.alphaMode == blend)
			{
				mTransparentMaterials.push_back({ const_cast<Renderer::Model::MaterialGroup&>(materialGroup), _model, _transform, occlusionKey });
			}

			auto [iterator, inserted] = mOcclusionCache.try_emplace(occlusionKey, OcclusionInfo{}); // Adds if not already there
			OcclusionInfo& occlusionInfo = iterator->second;

			if (inserted)
			{
				glGenQueries(2, occlusionInfo.queryIds);
			}

			occlusionInfo.lastFrameSubmitted = mFrameIndex;

			if (_shadow.mode != ShadowMode::Proxy)
			{
				mShadowMaterials.push_back({ const_cast<Renderer::Model::MaterialGroup&>(materialGroup), _model, _transform, occlusionKey });
			}
		}

		if (_shadow.mode == ShadowMode::Proxy)
		{
			for (const auto& materialGroup : _shadow.proxy->mModel->GetMaterialGroups())
			{
				mShadowMaterials.push_back({ const_cast<Renderer::Model::MaterialGroup&>(materialGroup), _shadow.proxy, _transform });
			}
		}
	}

	void SceneRenderer::ClearScene()
	{
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