#include "Component.h"

namespace JamesEngine
{

	std::shared_ptr<Entity> Component::GetEntity()
	{
		return mEntity.lock();
	}

	/*std::shared_ptr<Input> Component::GetInput()
	{
		return;
	}*/

	void Component::Tick()
	{
		OnTick();
	}

	void Component::Render()
	{
		OnRender();
	}

}