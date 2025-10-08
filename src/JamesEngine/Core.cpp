#include "Core.h"

#include "Entity.h"
#include "Transform.h"
#include "Resources.h"
#include "Input.h"
#include "Camera.h"
#include "Timer.h"
#include "Skybox.h"
#include "Texture.h"

#include <iostream>
#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "Shader.h"

namespace JamesEngine
{

	std::shared_ptr<Core> Core::Initialize(glm::ivec2 _windowSize)
	{
		std::shared_ptr<Core> rtn = std::make_shared<Core>();
		rtn->mWindow = std::make_shared<Window>(_windowSize.x, _windowSize.y);
		rtn->mAudio = std::make_shared<Audio>();
		rtn->mResources = std::make_shared<Resources>();
		rtn->mGUI = std::make_shared<GUI>(rtn);
		rtn->mLightManager = std::make_shared<LightManager>();
		rtn->mSkybox = std::make_shared<Skybox>(rtn);
		rtn->mRaycastSystem = std::make_shared<RaycastSystem>(rtn);
		rtn->mInput = std::make_shared<Input>();

		rtn->mSelf = rtn;

		return rtn;
	}

	void Core::SetLoadingScreen(std::shared_ptr<Texture> _texture)
	{
		mWindow->Update();
		mWindow->ClearWindow();

		int width, height;
		mWindow->GetWindowSize(width, height);

		// Needed now so that the loading screen UI can be rendered
		mGUI->PreUploadGlobalStaticUniformsUI();
		mGUI->UploadGlobalUniformsUI();

		mGUI->Image(glm::vec2(width/2, height/2), glm::vec2(width, height), _texture);

		mWindow->SwapWindows();
	}

	void Core::Run()
	{
		PreUploadGlobalStaticUniforms();
		mGUI->PreUploadGlobalStaticUniformsUI();

		Timer mDeltaTimer;

		while (mIsRunning)
		{
			//ScopedTimer timer("Core::Run");
			mDeltaTime = mDeltaTimer.Stop();

			//std::cout << "FPS: " << 1.0f / mDeltaTime << std::endl;

			mLastFrameTime = mDeltaTime;
			mDeltaTime *= mTimeScale;

			//std::cout << "deltaTime: " << mDeltaTime << std::endl;

			mDeltaTimer.Start();

			// 0 delta time (useful for loading a large model, takes a while so no long delta time)
			if (mDeltaTimeZero)
			{
				mDeltaTime = 0.0f;

				mDeltaTimeZeroCounter++;

				std::cout << "delta time is 0 for this frame" << std::endl;

				if (mDeltaTimeZeroCounter >= mNumDeltaTimeZeros)
					mDeltaTimeZero = false;
			}

			{
				//ScopedTimer timer("Core::HandleInputs");
				// Handle input events
				mInput->Update();

				SDL_Event event = {};
				while (SDL_PollEvent(&event))
				{
					if (event.type == SDL_QUIT)
					{
						mIsRunning = false;
					}
					else
					{
						mInput->HandleInput(event);
					}
				}
			}

			{
				//ScopedTimer timer("Core::Tick");
				// Run tick on all entities
				for (size_t ei = 0; ei < mEntities.size(); ++ei)
				{
					mEntities[ei]->OnTick();
				}
			}

			{
				// Fixed time step logic
				mFixedTimeAccumulator += mDeltaTime;

				if (mFixedTimeAccumulator > mFixedDeltaTime * 3) // Allow max 3 fixed updates per frame
					mFixedTimeAccumulator = mFixedDeltaTime * 3;

				int numFixedUpdates = 0;

				while (mFixedTimeAccumulator >= mFixedDeltaTime)
				{
					//ScopedTimer timer("Core::FixedTick");

					for (size_t ei = 0; ei < mEntities.size(); ++ei)
					{
						mEntities[ei]->OnEarlyFixedTick();
					}

					for (size_t ei = 0; ei < mEntities.size(); ++ei)
					{
						mEntities[ei]->OnFixedTick();
					}

					for (size_t ei = 0; ei < mEntities.size(); ++ei)
					{
						mEntities[ei]->OnLateFixedTick();
					}

					numFixedUpdates++;

					mFixedTimeAccumulator -= mFixedDeltaTime;
				}
			}

			// Delete entities that have been destroyed this frame
			for (size_t ei = 0; ei < mEntities.size(); ei++)
			{
				if (mEntities.at(ei)->mAlive == false)
				{
					mEntities.erase(mEntities.begin() + ei);
					ei--;
				}
			}

			// Clear the raycast cache (currently holds all colliders in current scene)
			mRaycastSystem->ClearCache();

			// Render the scene and GUI
			{
				//ScopedTimer timer("Core::RenderScene");
				RenderScene();
				RenderGUI();

				// Present the rendered frame
				mWindow->SwapWindows();
			}
		}
	}

	void Core::RenderScene()
	{
		//ScopedTimer timer("Core::InternalRenderScene");

		if (!mLightManager->ArePreBakedShadowsUploaded())
		{
			std::vector<std::shared_ptr<Renderer::RenderTexture>> preBakedShadowMaps;
			preBakedShadowMaps.reserve(mLightManager->GetPreBakedShadowMaps().size());
			std::vector<glm::mat4> preBakedShadowMatrices;
			preBakedShadowMatrices.reserve(mLightManager->GetPreBakedShadowMaps().size());

			for (const PreBakedShadowMap& shadowMap : mLightManager->GetPreBakedShadowMaps())
			{
				preBakedShadowMaps.emplace_back(shadowMap.renderTexture);
				preBakedShadowMatrices.emplace_back(shadowMap.lightSpaceMatrix);
			}

			mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_PreBakedShadowMaps", preBakedShadowMaps, 10);
			mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_PreBakedLightSpaceMatrices", preBakedShadowMatrices);

			mLightManager->SetPreBakedShadowsUploaded(true);

			std::cout << "Pre-baked shadows uploaded to shader" << std::endl;
		}

		if (!mLightManager->GetShadowCascades().empty())
		{
			// --- Render-state for depth-only shadow pass ---
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDisable(GL_MULTISAMPLE);
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

			// Camera info
			const glm::vec3 camPos = GetCamera()->GetPosition();
			const glm::vec3 camFwd = -GetCamera()->GetEntity()->GetComponent<Transform>()->GetForward();
			const glm::vec3 camUp = GetCamera()->GetEntity()->GetComponent<Transform>()->GetUp();
			const glm::vec3 camRight = GetCamera()->GetEntity()->GetComponent<Transform>()->GetRight();
			const float vfov = glm::radians(GetCamera()->GetFov());
			int winW, winH;
			mWindow->GetWindowSize(winW, winH);
			const float aspect = (winH > 0.0f) ? (winW / winH) : 16.0f / 9.0f;
			const float camNear = GetCamera()->GetNearClip();
			const float camFar = GetCamera()->GetFarClip();

			// Shadow configuration
			const float shadowDistance = 250;
			const float lambda = 0.6f; // 0..1 (e.g. 0.6)
			const glm::vec3 lightDir = glm::normalize(mLightManager->GetDirectionalLightDirection());

			auto& cascades = mLightManager->GetShadowCascades();
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

				// --- Render this cascade ---
				cascade.renderTexture->clear();
				cascade.renderTexture->bind();
				glViewport(0, 0, cascade.renderTexture->getWidth(), cascade.renderTexture->getHeight());

				for (size_t ei = 0; ei < mEntities.size(); ++ei)
				{
					mEntities[ei]->OnShadowRender(cascade.lightSpaceMatrix);
				}

				cascade.renderTexture->unbind();
			}

			mWindow->ResetGLModes();
		}

		// Prepare window for rendering
		mWindow->Update();
		mWindow->ClearWindow();

		mWindow->ResetGLModes();

		// Normal rendering of the scene
		mSkybox->RenderSkybox();

		UploadGlobalUniforms();

		for (size_t ei = 0; ei < mEntities.size(); ++ei)
		{
			mEntities[ei]->OnRender();
		}

		mWindow->ResetGLModes();
	}

	void Core::RenderGUI()
	{
		mWindow->ResetGLModes();

		mGUI->UploadGlobalUniformsUI();

		glDisable(GL_DEPTH_TEST);

		for (size_t ei = 0; ei < mEntities.size(); ++ei)
		{
			mEntities[ei]->OnGUI();
		}

		glEnable(GL_DEPTH_TEST);
	}

	void Core::PreUploadGlobalStaticUniforms()
	{
		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_DirLightDirection", mLightManager->GetDirectionalLightDirection()); // Assumes light direction and colour never change, light direction can't change while using pre baked shadows
		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_DirLightColor", mLightManager->GetDirectionalLightColour());

		mResources->Load<Shader>("shaders/ObjShader")->mShader->cubemapUniform("u_SkyBox", mSkybox->GetTexture()->mTexture, 29); // Assumes skybox doesn't change

		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_BRDFLUT", mResources->Load<Texture>("skyboxes/brdf_lut")->mTexture, 28);
	}

	void Core::UploadGlobalUniforms()
	{
		std::shared_ptr<Camera> camera = GetCamera();
		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_Projection", camera->GetProjectionMatrix());
		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_View", camera->GetViewMatrix());
		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_ViewPos", camera->GetPosition());

		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_Ambient", mLightManager->GetAmbient());

		// Wanted to upload shadow maps in the UploadGlobalUniforms function, but it needs to be done after the shadow maps are rendered
		std::vector<std::shared_ptr<Renderer::RenderTexture>> shadowMaps;
		shadowMaps.reserve(mLightManager->GetShadowCascades().size());
		std::vector<glm::mat4> shadowMatrices;
		shadowMatrices.reserve(mLightManager->GetShadowCascades().size());

		for (const ShadowCascade& cascade : mLightManager->GetShadowCascades())
		{
			shadowMaps.emplace_back(cascade.renderTexture);
			shadowMatrices.emplace_back(cascade.lightSpaceMatrix);
		}

		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_NumCascades", (int)shadowMaps.size());
		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_ShadowMaps", shadowMaps, 20);
		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_LightSpaceMatrices", shadowMatrices);

		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_NumPreBaked", 0);
	}

	std::shared_ptr<Entity> Core::AddEntity()
	{
		std::shared_ptr<Entity> rtn = std::make_shared<Entity>();
		rtn->AddComponent<Transform>();
		rtn->mSelf = rtn;
		rtn->mCore = mSelf;

		mEntities.push_back(rtn);

		return rtn;
	}

	// Returns the camera with the highest priority, if both have the same priority the first one found is returned
	std::shared_ptr<Camera> Core::GetCamera()
	{
		std::vector<std::shared_ptr<Camera>> cameras;
		FindComponents(cameras);

		if (cameras.size() == 0)
		{
			std::cout << "No entity with a camera component found" << std::endl;
			throw std::exception();
			return nullptr;
		}
		
		std::shared_ptr<Camera> rtn = cameras[0];
		for (size_t i = 1; i < cameras.size(); ++i)
		{
			if (cameras[i]->GetPriority() > rtn->GetPriority())
			{
				rtn = cameras[i];
			}
		}

		return rtn;
	}

	void Core::DestroyAllEntities()
	{
		for (size_t i = 0; i < mEntities.size(); ++i)
		{
			mEntities[i]->Destroy();
		}

		mDeltaTimeZero = true;
	}

	std::vector<std::shared_ptr<Entity>> Core::GetEntitiesByTag(std::string _tag)
	{
		std::vector<std::shared_ptr<Entity>> rtn;

		for (size_t i = 0; i < mEntities.size(); ++i)
		{
			if (mEntities[i]->GetTag() == _tag)
			{
				rtn.push_back(mEntities[i]);
			}
		}

		if (rtn.size() == 0)
		{
			std::cout << "No entities with tag " << _tag << " found" << std::endl;
		}

		return rtn;
	}

	std::shared_ptr<Entity> Core::GetEntityByTag(std::string _tag)
	{
		std::vector<std::shared_ptr<Entity>> rtn;

		for (size_t i = 0; i < mEntities.size(); ++i)
		{
			if (mEntities[i]->GetTag() == _tag)
			{
				return mEntities[i];
			}
		}

		std::cout << "No entity with tag " << _tag << " found" << std::endl;

		return nullptr;
	}

	bool Core::WriteTextFile(const std::string& _filePath, const std::string& _content)
	{
		std::filesystem::path p = std::filesystem::path("../assets/") / _filePath;

		// Ensure parent directories exist
		if (p.has_parent_path())
		{
			std::error_code ec;
			std::filesystem::create_directories(p.parent_path(), ec);
			if (ec)
			{
				std::cout << "Failed to create directories: " << p.parent_path().string() << " (" << ec.message() << ")\n";
				return false;
			}
		}

		// Open for overwrite, creates the file if it doesn't exist
		std::ofstream out(p, std::ios::binary | std::ios::trunc);
		if (!out)
		{
			std::cout << "Failed to open file for writing: " << p.string() << std::endl;
			return false;
		}

		if (!_content.empty())
		{
			out.write(_content.data(), static_cast<std::streamsize>(_content.size()));
		}
		out.flush();

		if (!out.good())
		{
			std::cout << "Write failed: " << p.string() << std::endl;
			return false;
		}

		out.close();

		// Confirm the file exists (should be true even for empty content)
		if (!std::filesystem::exists(p))
		{
			std::cout << "File was not created: " << p.string() << std::endl;
			return false;
		}

		return true;
	}

	bool Core::ReadTextFile(const std::string& _filePath, std::string& _outContent)
	{
		std::filesystem::path p = std::filesystem::path("../assets/") / _filePath;

		// Open in binary to read exact bytes, returns false if file is missing or can't be opened
		std::ifstream in(p, std::ios::binary);
		if (!in)
		{
			std::cout << "Failed to open file for reading: " << p.string() << std::endl;
			_outContent.clear();
			return false;
		}

		// Get size via filesystem
		std::error_code ec;
		const auto sz = std::filesystem::file_size(p, ec);
		if (ec)
		{
			std::cout << "Failed to get file size: " << p.string() << " (" << ec.message() << ")\n";
			_outContent.clear();
			return false;
		}

		_outContent.resize(static_cast<size_t>(sz));
		if (sz > 0)
		{
			in.read(&_outContent[0], static_cast<std::streamsize>(sz));
			if (!in)
			{
				std::cout << "Read failed: " << p.string() << std::endl;
				_outContent.clear();
				return false;
			}
		}
		return true;
	}

}