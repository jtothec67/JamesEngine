#include "Core.h"
#include "Entity.h"
#include "Window.h"

#include <iostream>

namespace JamesEngine
{

	std::shared_ptr<Core> Core::Initialize()
	{

		std::shared_ptr<Core> rtn = std::make_shared<Core>();
		rtn->mSelf = rtn;
		rtn->mWindow = std::make_shared<Window>(800, 600);
		return rtn;
	}

	void Core::Run()
	{
		//for (size_t i = 0; i < 25; ++i)
		while (true)
		{
			for (size_t ei = 0; ei < mEntities.size(); ++ei)
			{
				//mEntities[ei]->OnTick();
			}

			// WINDOW TEST CODE (WORKS)
			mWindow->Update();

			SDL_Event event = {};
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_QUIT)
				{
					return;
				}
			}

			mWindow->ClearWindow();

			mWindow->SwapWindows();
		}
	}

	std::shared_ptr<Entity> Core::AddEntity()
	{
		std::shared_ptr<Entity> rtn = std::make_shared<Entity>();
		rtn->mSelf = rtn;
		rtn->mCore = mSelf;

		mEntities.push_back(rtn);

		return rtn;
	}

}