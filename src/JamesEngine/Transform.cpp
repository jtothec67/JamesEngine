#include "Transform.h"

#include <glm/ext/matrix_transform.hpp>

namespace JamesEngine
{

	glm::mat4 Transform::GetModel()
	{
		glm::mat4 modelMatrix = glm::mat4(1.f);
		modelMatrix = glm::translate(modelMatrix, mPosition);
		modelMatrix = glm::rotate(modelMatrix, glm::radians(mRotation.x), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(mRotation.y), glm::vec3(0, 1, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(mRotation.z), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, mScale);

		return modelMatrix;
	}

	glm::vec3 Transform::GetForward()
	{
		glm::vec3 forward;
		forward.x = sin(glm::radians(mRotation.y)) * cos(glm::radians(mRotation.x));
		forward.y = sin(glm::radians(mRotation.x));
		forward.z = cos(glm::radians(mRotation.y)) * cos(glm::radians(mRotation.x));

		return glm::normalize(forward);
	}

	glm::vec3 Transform::GetRight()
	{
		glm::vec3 right;
		right.x = sin(glm::radians(mRotation.y - 90)) * cos(glm::radians(mRotation.x));
		right.y = sin(glm::radians(mRotation.x));
		right.z = cos(glm::radians(mRotation.y - 90)) * cos(glm::radians(mRotation.x));

		return glm::normalize(right);
	}

	glm::vec3 Transform::GetUp()
	{
		glm::vec3 up;
		up.x = sin(glm::radians(mRotation.y)) * cos(glm::radians(mRotation.x + 90));
		up.y = sin(glm::radians(mRotation.x + 90));
		up.z = cos(glm::radians(mRotation.y)) * cos(glm::radians(mRotation.x + 90));

		return glm::normalize(up);
	}

}