#include "Component.h"

namespace JamesEngine
{
	void Component::OnInitialize() { }
	void Component::OnTick() { }

	void Component::Tick()
	{
		OnTick();
	}
}