#include "Core.h"

#include "Entity.h"
#include "Transform.h"
#include "Resources.h"
#include "Input.h"
#include "Camera.h"
#include "Timer.h"
#include "Skybox.h"

#include <iostream>

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

	void Core::Run()
	{
		Timer mDeltaTimer;

		mLightManager->InitShadowMap();
		Renderer::RenderTexture* shadowMap = mLightManager->GetDirectionalLightShadowMap();

		shadowMap->clear();

		while (mIsRunning)
		{
			mDeltaTime = mDeltaTimer.Stop();

			//std::cout << "FPS: " << 1.0f / mDeltaTime << std::endl;

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

			// Run tick on all entities
			for (size_t ei = 0; ei < mEntities.size(); ++ei)
			{
				mEntities[ei]->OnTick();
			}

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
			RenderScene();
			RenderGUI();

			// Present the rendered frame
			mWindow->SwapWindows();
		}
	}

	void Core::RenderScene()
	{
		// Prepare the shadow map for rendering
		glm::vec3 camPos = GetCamera()->GetPosition();
		glm::vec3 camForward = GetCamera()->GetEntity()->GetComponent<Transform>()->GetForward();

		glm::vec2 orthoSize = mLightManager->GetOrthoShadowMapSize();
		float nearPlane = mLightManager->GetDirectionalLightNearPlane();
		float farPlane = mLightManager->GetDirectionalLightFarPlane();

		// Project the forward vector onto the XZ plane (remove vertical component)
		glm::vec3 flatForward = glm::normalize(glm::vec3(-camForward.x, 0.0f, -camForward.z));

		// Scene center is now ahead of camera, so more of camera's view is in the shadow map
		glm::vec3 sceneCenter = camPos + flatForward * orthoSize.x/1.25f;

		glm::vec3 lightDir = glm::normalize(mLightManager->GetDirectionalLightDirection());
		// Calculate light position, centered around the camera, positioned in direction of light half the far view plane away
		glm::vec3 lightPos = sceneCenter - lightDir * farPlane/2.f;

		// Look at the camera
		glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0, 1, 0));

		glm::mat4 lightProjection = glm::ortho(
			-orthoSize.x, orthoSize.x,
			-orthoSize.y, orthoSize.y,
			nearPlane, farPlane
		);

		// Store the light space matrix for shadow mapping rendering
		mLightSpaceMatrix = lightProjection * lightView;

		// Draw the scene on to the shadow map
		Renderer::RenderTexture* shadowMap = mLightManager->GetDirectionalLightShadowMap();

		// Clear from last frame
		shadowMap->clear();

		shadowMap->bind();
		glViewport(0, 0, shadowMap->getWidth(), shadowMap->getHeight());

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);

		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_MULTISAMPLE);
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

		for (size_t ei = 0; ei < mEntities.size(); ++ei)
		{
			// Special render to use the depth only shader and upload appropriate uniforms
			mEntities[ei]->OnShadowRender();
		}

		shadowMap->unbind();

		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

		// Prepare window for rendering
		mWindow->Update();
		mWindow->ClearWindow();

		// Normal rendering of the scene
		mSkybox->RenderSkybox();

		for (size_t ei = 0; ei < mEntities.size(); ++ei)
		{
			mEntities[ei]->OnRender();
		}
	}

	void Core::RenderGUI()
	{
		glDisable(GL_DEPTH_TEST);

		for (size_t ei = 0; ei < mEntities.size(); ++ei)
		{
			mEntities[ei]->OnGUI();
		}

		glEnable(GL_DEPTH_TEST);
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