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
				//ScopedTimer timer("Core::FixedTick");

				// Fixed time step logic
				mFixedTimeAccumulator += mDeltaTime;

				if (mFixedTimeAccumulator > mFixedDeltaTime * 3) // Allow max 3 fixed updates per frame
					mFixedTimeAccumulator = mFixedDeltaTime * 3;

				int numFixedUpdates = 0;

				while (mFixedTimeAccumulator >= mFixedDeltaTime)
				{
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
			// Render shadow maps
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDisable(GL_MULTISAMPLE);
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

			glm::vec3 camPos = GetCamera()->GetPosition();
			glm::vec3 camForward = GetCamera()->GetEntity()->GetComponent<Transform>()->GetForward();
			glm::vec3 flatForward = glm::normalize(glm::vec3(-camForward.x, 0.0f, -camForward.z));

			for (ShadowCascade& cascade : mLightManager->GetShadowCascades())
			{
				//ScopedTimer timerCascade("Core::RenderScene::ShadowCascade");
				glm::vec2 orthoSize = cascade.orthoSize;
				float nearPlane = cascade.nearPlane;
				float farPlane = cascade.farPlane;

				glm::vec3 sceneCenter = camPos + flatForward * (orthoSize.x * 1.f);

				glm::vec3 lightDir = glm::normalize(mLightManager->GetDirectionalLightDirection());
				glm::vec3 lightPos = sceneCenter - lightDir * (farPlane / 2.f);

				glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0, 1, 0));

				glm::mat4 lightProjection = glm::ortho(
					-orthoSize.x, orthoSize.x,
					-orthoSize.y, orthoSize.y,
					nearPlane, farPlane
				);

				// 3. Compute light-space matrix
				glm::mat4 lightSpaceMatrix = lightProjection * lightView;

				// 4. Snap scene center to texel grid
				// Estimate world units per texel
				float shadowMapResolution = static_cast<float>(cascade.resolution.x);
				float texelSize = (orthoSize.x * 2.0f) / shadowMapResolution;

				// Transform origin into light space
				glm::vec4 originLS = lightSpaceMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

				// Snap to nearest texel
				originLS.x = texelSize * std::round(originLS.x / texelSize);
				originLS.y = texelSize * std::round(originLS.y / texelSize);

				// Calculate offset between original and snapped origin
				glm::vec3 offset = glm::vec3(originLS.x, originLS.y, 0.0f) - glm::vec3(lightSpaceMatrix[3]);

				// Apply offset to light projection matrix
				lightProjection[3][0] += offset.x;
				lightProjection[3][1] += offset.y;

				cascade.lightSpaceMatrix = lightProjection * lightView;

				cascade.renderTexture->clear();
				cascade.renderTexture->bind();
				glViewport(0, 0, cascade.renderTexture->getWidth(), cascade.renderTexture->getHeight());

				for (size_t ei = 0; ei < mEntities.size(); ++ei)
				{
					mEntities[ei]->OnShadowRender(cascade.lightSpaceMatrix);
				}

				cascade.renderTexture->unbind();
			}

			//ScopedTimer sceneTimer("Core::Final render to screen");

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

		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_ShadowMaps", shadowMaps, 20);
		mResources->Load<Shader>("shaders/ObjShader")->mShader->uniform("u_LightSpaceMatrices", shadowMatrices);
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

}