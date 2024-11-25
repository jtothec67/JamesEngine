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

}