#pragma once

#include <memory>

namespace JamesEngine
{

	class Entity;
	class Input;

	class Component
	{
	public:
		std::shared_ptr<Entity> GetEntity();
		//std::shared_ptr<Input> GetInput();

		virtual void OnInitialize() { }
		virtual void OnTick() { }
		virtual void OnRender() { }

	private:
		friend class JamesEngine::Entity;

		std::weak_ptr<Entity> mEntity;

		void Tick();
		void Render();
	};
}