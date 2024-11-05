#include "Entity.h"

#include "Component.h"

namespace JamesEngine
{
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