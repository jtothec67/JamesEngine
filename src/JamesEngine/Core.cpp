#include "Core.h"
#include "Entity.h"
#include "Window.h"

#include <iostream>

namespace JamesEngine
{

	std::shared_ptr<Core> Core::Initialize()
	{

		std::shared_ptr<Core> rtn = std::make_shared<Core>();
		rtn->mWindow = std::make_shared<Window>();
		rtn->mSelf = rtn;

		return rtn;
	}

	void Core::Run()
	{
		mWindow->OnInitialize(800, 600);
		
		while (true)
		{
			for (size_t ei = 0; ei < mEntities.size(); ++ei)
			{
				mEntities[ei]->OnTick();
			}

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
		rtn->mSelf = rtn;
		rtn->mCore = mSelf;

		mEntities.push_back(rtn);

		return rtn;
	}

}