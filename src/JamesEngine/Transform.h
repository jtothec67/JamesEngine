#pragma once

#include "Component.h"

#include <glm/glm.hpp>

namespace JamesEngine
{
	class Transform : public Component
	{
	public:
		glm::vec3 position{ 0.f };
		glm::vec3 rotation{ 0.f };
		glm::vec3 scale{ 1.f };

		glm::vec3 GetForward()
		{
			glm::vec3 forward;
			forward.x = sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
			forward.y = sin(glm::radians(rotation.x));
			forward.z = cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));

			return glm::normalize(forward);
		}

		glm::vec3 GetRight()
		{
			glm::vec3 right;
			right.x = sin(glm::radians(rotation.y - 90)) * cos(glm::radians(rotation.x));
			right.y = sin(glm::radians(rotation.x));
			right.z = cos(glm::radians(rotation.y - 90)) * cos(glm::radians(rotation.x));

			return glm::normalize(right);
		}

		glm::vec3 GetUp()
		{
			glm::vec3 up;
			up.x = sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.x + 90));
			up.y = sin(glm::radians(rotation.x + 90));
			up.z = cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x + 90));

			return glm::normalize(up);
		}
	};
}