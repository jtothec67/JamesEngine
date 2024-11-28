#include "Core.h"
#include "Entity.h"
#include "Transform.h"
#include "Resources.h"

#include <iostream>

namespace JamesEngine
{

	std::shared_ptr<Core> Core::Initialize()
	{
		std::shared_ptr<Core> rtn = std::make_shared<Core>();
		rtn->mWindow = std::make_shared<Window>(800, 600);
		rtn->mResources = std::make_shared<Resources>();
		rtn->mInput = std::make_shared<Input>();
		rtn->mSelf = rtn;

		return rtn;
	}

	void Core::Run()
	{
		while (true)
		{
			if (!mInput->Update())
			{
				return;
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