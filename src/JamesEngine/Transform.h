#pragma once

#include "Component.h"

#include <glm/glm.hpp>

namespace JamesEngine
{
	class Transform : public Component
	{
	public:
		void SetPosition(glm::vec3 _position) { mPosition = _position; }
		glm::vec3 GetPosition() { return mPosition; }

		void SetRotation(glm::vec3 _rotation) { mRotation = _rotation; }
		glm::vec3 GetRotation() { return mRotation; }

		void SetScale(glm::vec3 _scale) { mScale = _scale; }
		glm::vec3 GetScale() { return mScale; }

		glm::mat4 GetModel();

		glm::vec3 GetForward()
		{
			glm::vec3 forward;
			forward.x = sin(glm::radians(mRotation.y)) * cos(glm::radians(mRotation.x));
			forward.y = sin(glm::radians(mRotation.x));
			forward.z = cos(glm::radians(mRotation.y)) * cos(glm::radians(mRotation.x));

			return glm::normalize(forward);
		}

		glm::vec3 GetRight()
		{
			glm::vec3 right;
			right.x = sin(glm::radians(mRotation.y - 90)) * cos(glm::radians(mRotation.x));
			right.y = sin(glm::radians(mRotation.x));
			right.z = cos(glm::radians(mRotation.y - 90)) * cos(glm::radians(mRotation.x));

			return glm::normalize(right);
		}

		glm::vec3 GetUp()
		{
			glm::vec3 up;
			up.x = sin(glm::radians(mRotation.y)) * cos(glm::radians(mRotation.x + 90));
			up.y = sin(glm::radians(mRotation.x + 90));
			up.z = cos(glm::radians(mRotation.y)) * cos(glm::radians(mRotation.x + 90));

			return glm::normalize(up);
		}

	private:
		glm::vec3 mPosition{ 0.f };
		glm::vec3 mRotation{ 0.f };
		glm::vec3 mScale{ 1.f };
	};
}