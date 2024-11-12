#include "Component.h"

namespace JamesEngine
{
	void Component::Tick()
	{
		OnTick();
	}

	void Component::Render()
	{
		OnRender();
	}
}