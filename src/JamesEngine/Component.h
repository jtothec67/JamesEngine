#pragma once

#include <glm/glm.hpp>

#include <memory>

namespace JamesEngine
{

	class Entity;
	class Input;
	class Keyboard;
	class Mouse;
	class Transform;

	class Component
	{
	public:
		std::shared_ptr<Entity> GetEntity();
		std::shared_ptr<Input> GetInput();
		std::shared_ptr<Keyboard> GetKeyboard();
		std::shared_ptr<Mouse> GetMouse();
		std::shared_ptr<Transform> GetTransform();
		glm::vec3 GetPosition();
		void SetPosition(glm::vec3 _position);

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