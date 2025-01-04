#include "Core.h"

#include "Entity.h"
#include "Transform.h"
#include "Resources.h"
#include "Input.h"
#include "Camera.h"
#include "Timer.h"

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
		rtn->mInput = std::make_shared<Input>();
		rtn->mSelf = rtn;

		return rtn;
	}

	void Core::Run()
	{
		Timer mDeltaTimer;

		while (mIsRunning)
		{
			mDeltaTime = mDeltaTimer.Stop();
			mDeltaTimer.Start();

			//std::cout << "FPS: " << 1.0f / mDeltaTime << std::endl;

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

			for (size_t ei = 0; ei < mEntities.size(); ++ei)
			{
				mEntities[ei]->OnTick();
			}

			mWindow->Update();

			mWindow->ClearWindow();

			for (size_t ei = 0; ei < mEntities.size(); ++ei)
			{
				mEntities[ei]->OnRender();
			}

			glDisable(GL_DEPTH_TEST);

			for (size_t ei = 0; ei < mEntities.size(); ++ei)
			{
				mEntities[ei]->OnGUI();
			}

			glEnable(GL_DEPTH_TEST);

			mWindow->SwapWindows();
		}
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

}