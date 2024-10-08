#include "Core.h"
#include "Entity.h"

#include <iostream>

namespace JamesEngine
{

	std::shared_ptr<Core> Core::Initialize()
	{
		std::shared_ptr<Core> rtn = std::make_shared<Core>();
		rtn->mSelf = rtn;
		return rtn;
	}

	void Core::Start()
	{
		for (size_t i = 0; i < 25; ++i)
		{

		}
	}

	std::shared_ptr<Entity> Core::AddEntity()
	{
		std::shared_ptr<Entity> rtn = std::make_shared<Entity>();
		rtn->mCore = mSelf;

		mEntities.push_back(rtn);

		return rtn;
	}

}