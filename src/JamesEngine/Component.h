#pragma once

#include <memory>

namespace JamesEngine
{

	class Entity;

	class Component
	{
	public:
		std::shared_ptr<Entity> GetEntity() { return std::make_shared<Entity>(mEntity); }

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