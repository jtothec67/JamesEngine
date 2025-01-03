#pragma once

#include <glm/glm.hpp>

#include <memory>

namespace JamesEngine
{

	class Entity;
	class Input;
	class Keyboard;
	class Mouse;
	class GUI;
	class Transform;

	class Component
	{
	public:
		std::shared_ptr<Entity> GetEntity();
		std::shared_ptr<Input> GetInput();
		std::shared_ptr<Keyboard> GetKeyboard();
		std::shared_ptr<Mouse> GetMouse();
		std::shared_ptr<GUI> GetGUI();
		std::shared_ptr<Transform> GetTransform();

		glm::vec3 GetPosition();
		void SetPosition(glm::vec3 _position);
		glm::vec3 GetRotation();
		void SetRotation(glm::vec3 _rotation);
		glm::vec3 GetScale();
		void SetScale(glm::vec3 _scale);
		void Move(glm::vec3 _amount);
		void Rotate(glm::vec3 _amount);

		virtual void OnInitialize() { }
		virtual void OnTick() { }
		virtual void OnRender() { }
		virtual void OnGUI() { }

	private:
		friend class JamesEngine::Entity;

		std::weak_ptr<Entity> mEntity;

		void Tick();
		void Render();
		void GUI();
	};
}