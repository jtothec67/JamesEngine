#include "Component.h"

namespace JamesEngine
{

	std::shared_ptr<Entity> Component::GetEntity()
	{
		return mEntity.lock();
	}

	void Component::Tick()
	{
		OnTick();
	}

	void Component::Render()
	{
		OnRender();
	}

}