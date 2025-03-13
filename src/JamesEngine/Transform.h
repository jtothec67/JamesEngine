#pragma once

#include "Component.h"

#include <glm/glm.hpp>

namespace JamesEngine
{
	class Transform : public Component
	{
	public:
		glm::mat4 GetModel();

		void SetParent(std::shared_ptr<Entity> _parent) { mParent = _parent; }

		void SetPosition(glm::vec3 _position) { mPosition = _position; }
		glm::vec3 GetPosition();

		void SetRotation(glm::vec3 _rotation) 
		{ 
			while (_rotation.x > 360)
				_rotation.x -= 720;

			while (_rotation.x < -360)
				_rotation.x += 720;

			while (_rotation.y > 360)
				_rotation.y -= 720;

			while (_rotation.y < -360)
				_rotation.y += 720;

			while (_rotation.z > 360)
				_rotation.z -= 720;

			while (_rotation.z < -360)
				_rotation.z += 720;

			mRotation = _rotation;
		}
		glm::vec3 GetRotation();

		void SetScale(glm::vec3 _scale) { mScale = _scale; }
		glm::vec3 GetScale();

		glm::vec3 GetForward();
		glm::vec3 GetRight();
		glm::vec3 GetUp();

		void Move(glm::vec3 _amount) { mPosition += _amount; }
		void Rotate(glm::vec3 _amount) { mRotation += _amount; }

	private:
		glm::vec3 mPosition{ 0.f };
		glm::vec3 mRotation{ 0.f };
		glm::vec3 mScale{ 1.f };

		std::weak_ptr<Entity> mParent;
	};
}