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
		//core->FindComponents<Camera>(cams); // FOR DEBUGGING - can see the scene from one veiw while still processes it from another (I know the first camera is a free roam cam)
		//mObjShader->mShader->uniform("u_View", cams.at(0)->GetViewMatrix());
		//mObjShader->mShader->uniform("u_ViewPos", cams.at(0)->GetPosition());
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
			const float aspect = (winH > 0) ? (static_cast<float>(winW) / static_cast<float>(winH)) : (16.0f / 9.0f);
			const float camNear = camera->GetNearClip();
			const float camFar = camera->GetFarClip();

			// Shadow configuration
			const float shadowDistance = 250;
			const float lambda = 0.6f; // 0..1 (e.g. 0.6)
			const glm::vec3 lightDir = -glm::normalize(core->mLightManager->GetDirectionalLightDirection()); // Reverse to get light "forward"

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
			const float pcfMarginXY = 0.0f;
			const float casterMarginZ = 500.0f;

			float prevFar = camNear;

			// SHADOW CASCADE RENDERING
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

				// 2) Light view with XY aligned to the camera’s screen axes.
				// This keeps the ortho rectangle “in front of you” instead of sliding behind
				// when the view is diagonal relative to the light.
				const float eyeDist = 1000.0f;

				glm::vec3 z = -glm::normalize(lightDir); // light forward in view space
				// Project camera right onto plane perpendicular to light to get a stable X
				glm::vec3 x = camRight - z * glm::dot(camRight, z);
				if (glm::length(x) < 1e-6f)
				{
					// Fallback if camRight ~ parallel to light; use camUp instead
					x = camUp - z * glm::dot(camUp, z);
				}
				x = glm::normalize(x);

				glm::vec3 y = glm::normalize(glm::cross(z, x)); // guaranteed orthogonal and stable

				// Build view matrix anchored at the slice center, using our orthogonal basis
				glm::mat4 lightView = glm::lookAt(center - z * eyeDist, center, y);

				// 3) Transform corners to light space and fit a tight AABB
				glm::vec3 minLS(+FLT_MAX), maxLS(-FLT_MAX);
				for (auto& p : cornersWS)
				{
					glm::vec4 q = lightView * glm::vec4(p, 1.0f);
					minLS = glm::min(minLS, glm::vec3(q));
					maxLS = glm::max(maxLS, glm::vec3(q));
				}

				const float resX = float(cascade.renderTexture->getWidth());
				const float resY = float(cascade.renderTexture->getHeight());

				const float texelX = (maxLS.x - minLS.x) / resX;
				const float texelY = (maxLS.y - minLS.y) / resY;

				minLS.x = std::floor(minLS.x / texelX) * texelX;
				minLS.y = std::floor(minLS.y / texelY) * texelY;
				maxLS.x = std::floor(maxLS.x / texelX) * texelX;
				maxLS.y = std::floor(maxLS.y / texelY) * texelY;

				minLS.z -= casterMarginZ;     // extend toward the light
				maxLS.z += casterMarginZ;     // extend away from the light

				// 5) Ortho projection from this AABB (OpenGL NDC Z = [-1,1])
				glm::mat4 lightProj = glm::ortho(minLS.x, maxLS.x,
					minLS.y, maxLS.y,
					-maxLS.z, -minLS.z);

				// 6) Final matrix for the shader
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

				// Build a copy sorted by distance (closest -> furthest)
				const glm::mat4 V = lightView;
				const glm::mat4 P = lightProj;
				const glm::mat4 VP = cascade.lightSpaceMatrix;

				// Extract world-space frustum planes from VP (GLM is column-major)
				struct Plane { glm::vec3 n; float d; }; // plane: n·x + d >= 0  => inside
				auto getRow = [&](int r) {
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
				for (Plane& p : planes) {
					float invLen = 1.0f / glm::length(p.n);
					p.n *= invLen;
					p.d *= invLen;
				}

				std::vector<std::pair<float, size_t>> keys;
				keys.reserve(mShadowMaterials.size());

				for (size_t i = 0; i < mShadowMaterials.size(); ++i)
				{
					const auto& it = mShadowMaterials[i];

					// --- OBB in WORLD space
					const glm::mat4  M = it.transform;
					const glm::vec3  e = it.materialGroup.boundsHalfExtentsMS;
					const glm::vec3  cW = glm::vec3(M * glm::vec4(it.materialGroup.boundsCenterMS, 1.0f));
					const glm::mat3  A = glm::mat3(M); // columns are world axes * scale

					// Conservative frustum cull (OBB vs plane SAT, world-space planes)
					// Inside if n·c + d >= -r, where r = sum |n·axis_i| * e_i
					bool culled = false;
					for (const Plane& pl : planes)
					{
						const float r =
							std::abs(glm::dot(pl.n, A[0])) * e.x +
							std::abs(glm::dot(pl.n, A[1])) * e.y +
							std::abs(glm::dot(pl.n, A[2])) * e.z;

						const float s = glm::dot(pl.n, cW) + pl.d;

						if (s < -r) { // fully outside this plane -> reject
							culled = true;
							break;
						}
					}
					if (culled) continue;

					// Depth key (front-to-back)
					const glm::mat4 VM = V * M;
					const glm::vec3 centerVS = glm::vec3(VM * glm::vec4(it.materialGroup.boundsCenterMS, 1.0f));
					const glm::mat3 AV = glm::mat3(VM);
					const float projZ = std::abs(AV[0][2]) * e.x + std::abs(AV[1][2]) * e.y + std::abs(AV[2][2]) * e.z;
					const float zMin = centerVS.z - projZ;

					keys.emplace_back(zMin, i);
				}

				// OPAQUE: front->back => zMin ascending (most negative first)
				std::sort(keys.begin(), keys.end(),
					[](const auto& a, const auto& b) { return a.first < b.first; });

				std::vector<MaterialRenderInfo> distanceSortedShadowMaterials;
				distanceSortedShadowMaterials.reserve(keys.size());
				for (const auto& kv : keys)
					distanceSortedShadowMaterials.push_back(mShadowMaterials[kv.second]);

				// Avoids copying or double-deleting the underlying GL object.
				auto asShared = [](const Renderer::Texture& t) -> std::shared_ptr<Renderer::Texture> {
					return std::shared_ptr<Renderer::Texture>(const_cast<Renderer::Texture*>(&t), [](Renderer::Texture*) {});
					};

				// Render all shadow-casting objects
				for (const auto& shadowMaterial : distanceSortedShadowMaterials)
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
		const glm::mat4 VP = camProj * camView;

		// Extract world-space frustum planes from VP (GLM is column-major)
		struct Plane { glm::vec3 n; float d; }; // plane: n·x + d >= 0  => inside
		auto getRow = [&](int r) {
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
		for (Plane& p : planes) {
			float invLen = 1.0f / glm::length(p.n);
			p.n *= invLen;
			p.d *= invLen;
		}

		// Frustum cull and sort OPAQUE materials back-to-front
		std::vector<std::pair<float, size_t>> opaqueKeys;
		opaqueKeys.reserve(mOpaqueMaterials.size());

		for (size_t i = 0; i < mOpaqueMaterials.size(); ++i)
		{
			const auto& it = mOpaqueMaterials[i];

			// OBB in WORLD space
			const glm::mat4  M = it.transform;
			const glm::vec3  e = it.materialGroup.boundsHalfExtentsMS;
			const glm::vec3  cW = glm::vec3(M * glm::vec4(it.materialGroup.boundsCenterMS, 1.0f));
			const glm::mat3  A = glm::mat3(M);

			// Conservative frustum cull (OBB vs plane SAT, world-space planes)
			// Inside if n·c + d >= -r, where r = sum |n·axis_i| * e_i
			bool culled = false;
			for (const Plane& pl : planes)
			{
				const float r =
					std::abs(glm::dot(pl.n, A[0])) * e.x +
					std::abs(glm::dot(pl.n, A[1])) * e.y +
					std::abs(glm::dot(pl.n, A[2])) * e.z;

				const float s = glm::dot(pl.n, cW) + pl.d;

				if (s < -r) { // Fully outside this plane -> reject
					culled = true;
					break;
				}
			}
			if (culled) continue;

			// Depth key (front-to-back)
			const glm::mat4 VM = camView * M;
			const glm::vec3 centerVS = glm::vec3(VM * glm::vec4(it.materialGroup.boundsCenterMS, 1.0f));
			const glm::mat3 AV = glm::mat3(VM);
			const float projZ = std::abs(AV[0][2]) * e.x + std::abs(AV[1][2]) * e.y + std::abs(AV[2][2]) * e.z;
			const float zMax = centerVS.z + projZ;

			opaqueKeys.emplace_back(zMax, i);
		}

		// OPAQUE: front->back => zMin ascending (most negative first)
		std::sort(opaqueKeys.begin(), opaqueKeys.end(),
			[](const auto& a, const auto& b) { return a.first > b.first; });

		std::vector<MaterialRenderInfo> distanceSortedOpaqueMaterials;
		distanceSortedOpaqueMaterials.reserve(opaqueKeys.size());
		for (const auto& kv : opaqueKeys)
			distanceSortedOpaqueMaterials.push_back(mOpaqueMaterials[kv.second]);

		// Frustum cull and sort TRANSPARENT materials back-to-front
		std::vector<std::pair<float, size_t>> transparentKeys;
		transparentKeys.reserve(mTransparentMaterials.size());

		for (size_t i = 0; i < mTransparentMaterials.size(); ++i)
		{
			const auto& it = mTransparentMaterials[i];

			// OBB in WORLD space
			const glm::mat4  M = it.transform;
			const glm::vec3  e = it.materialGroup.boundsHalfExtentsMS;
			const glm::vec3  cW = glm::vec3(M * glm::vec4(it.materialGroup.boundsCenterMS, 1.0f));
			const glm::mat3  A = glm::mat3(M);

			// Conservative frustum cull (OBB vs plane SAT, world-space planes)
			// Inside if n·c + d >= -r, where r = sum |n·axis_i| * e_i
			bool culled = false;
			for (const Plane& pl : planes)
			{
				const float r =
					std::abs(glm::dot(pl.n, A[0])) * e.x +
					std::abs(glm::dot(pl.n, A[1])) * e.y +
					std::abs(glm::dot(pl.n, A[2])) * e.z;

				const float s = glm::dot(pl.n, cW) + pl.d;

				if (s < -r) { // Fully outside this plane -> reject
					culled = true;
					break;
				}
			}
			if (culled) continue;

			// Depth key (front-to-back)
			const glm::mat4 VM = camView * M;
			const glm::vec3 centerVS = glm::vec3(VM * glm::vec4(it.materialGroup.boundsCenterMS, 1.0f));
			const glm::mat3 AV = glm::mat3(VM);
			const float projZ = std::abs(AV[0][2]) * e.x + std::abs(AV[1][2]) * e.y + std::abs(AV[2][2]) * e.z;
			const float zMin = centerVS.z - projZ;
			transparentKeys.emplace_back(zMin, i);
		}

		std::sort(transparentKeys.begin(), transparentKeys.end(),
			[](const auto& a, const auto& b) { return a.first < b.first; });

		std::vector<MaterialRenderInfo> distanceSortedTransparentMaterials;
		distanceSortedTransparentMaterials.reserve(transparentKeys.size());
		for (const auto& kv : transparentKeys)
			distanceSortedTransparentMaterials.push_back(mTransparentMaterials[kv.second]);

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

		// Fallbacks
		mObjShader->mShader->uniform("u_AlbedoFallback", mBaseColorStrength);
		mObjShader->mShader->uniform("u_MetallicFallback", mMetallicness);
		mObjShader->mShader->uniform("u_RoughnessFallback", mRoughness);
		mObjShader->mShader->uniform("u_AOFallback", mAOFallback);
		mObjShader->mShader->uniform("u_EmissiveFallback", mEmmisive);

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

}