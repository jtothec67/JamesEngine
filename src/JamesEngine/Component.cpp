#include "Component.h"
#include "Entity.h"
#include "Transform.h"
#include "Core.h"

namespace JamesEngine
{

	std::shared_ptr<Entity> Component::GetEntity()
	{
		return mEntity.lock();
	}

	std::shared_ptr<Input> Component::GetInput()
	{
		return GetEntity()->GetCore()->GetInput();
	}

	std::shared_ptr<Transform> Component::GetTransform()
	{
		return GetEntity()->GetComponent<Transform>();
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