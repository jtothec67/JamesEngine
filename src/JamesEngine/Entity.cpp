#include "Entity.h"

#include "Component.h"
#include "Core.h"

namespace JamesEngine
{
	std::shared_ptr<Core> Entity::GetCore()
	{
		return mCore.lock();
	}

	void Entity::OnTick()
	{
		for (size_t ci = 0; ci < mComponents.size(); ++ci)
		{
			mComponents.at(ci)->Tick();
		}
	}

	void Entity::OnRender()
	{
		for (size_t ci = 0; ci < mComponents.size(); ++ci)
		{
			mComponents.at(ci)->Render();
		}
	}
}