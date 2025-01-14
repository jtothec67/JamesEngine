#include "Transform.h"
#include "Entity.h"

#include <glm/ext/matrix_transform.hpp>

namespace JamesEngine
{

	glm::vec3 Transform::GetPosition()
	{
		if (auto parent = mParent.lock()) // Use weak_ptr's lock() to get shared_ptr
		{
			glm::vec3 parentPosition = parent->GetComponent<Transform>()->GetPosition();
			glm::vec3 parentRotation = parent->GetComponent<Transform>()->GetRotation();

			// Create a rotation matrix from the parent's rotation
			glm::mat4 rotationMatrix = glm::mat4(1.0f);
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(parentRotation.x), glm::vec3(1, 0, 0));
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(parentRotation.y), glm::vec3(0, 1, 0));
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(parentRotation.z), glm::vec3(0, 0, 1));

			// Apply the parent's rotation to the child's position
			glm::vec4 rotatedPosition = rotationMatrix * glm::vec4(mPosition, 1.0f);

			return glm::vec3(rotatedPosition) + parentPosition;
		}

		return mPosition;
	}

	glm::vec3 Transform::GetRotation()
	{
		if (auto parent = mParent.lock())
			return mRotation + parent->GetComponent<Transform>()->GetRotation();

		return mRotation;
	}

	glm::vec3 Transform::GetScale()
	{
		if (auto parent = mParent.lock())
			return mScale * parent->GetComponent<Transform>()->GetScale();

		return mScale;
	}

    glm::mat4 Transform::GetModel()
    {
		glm::mat4 modelMatrix = glm::mat4(1.f);
		modelMatrix = glm::translate(modelMatrix, GetPosition());
		glm::vec3 rotation = GetRotation();
		modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, GetScale());

		/*if (auto parent = mParent.lock())
			return parent->GetComponent<Transform>()->GetModel() * modelMatrix;*/

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