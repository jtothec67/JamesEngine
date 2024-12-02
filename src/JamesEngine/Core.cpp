#include "Core.h"
#include "Entity.h"
#include "Transform.h"
#include "Resources.h"
#include "Input.h"

#include <iostream>

namespace JamesEngine
{

	std::shared_ptr<Core> Core::Initialize(glm::ivec2 _windowSize)
	{
		std::shared_ptr<Core> rtn = std::make_shared<Core>();
		rtn->mWindow = std::make_shared<Window>(_windowSize.x, _windowSize.y);
		rtn->mAudio = std::make_shared<Audio>();
		rtn->mResources = std::make_shared<Resources>();
		rtn->mInput = std::make_shared<Input>();
		rtn->mSelf = rtn;

		return rtn;
	}

	void Core::Run()
	{
		while (true)
		{
			mInput->Update();
			
			SDL_Event event = {};
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_QUIT)
				{
					return;
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

}