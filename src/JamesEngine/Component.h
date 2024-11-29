#pragma once

#include <memory>

namespace JamesEngine
{

	class Entity;
	class Input;
	class Transform;

	class Component
	{
	public:
		std::shared_ptr<Entity> GetEntity();
		std::shared_ptr<Input> GetInput();
		std::shared_ptr<Transform> GetTransform();

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