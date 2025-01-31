#include "SphereCollider.h"

#include "Core.h"
#include "BoxCollider.h"

#ifdef _DEBUG
#include "Camera.h"
#include "Entity.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

namespace JamesEngine
{

#ifdef _DEBUG
	void SphereCollider::OnGUI()
	{
		std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();

		mShader->uniform("projection", camera->GetProjectionMatrix());

		mShader->uniform("view", camera->GetViewMatrix());

		glm::mat4 mModelMatrix = glm::mat4(1.f);
		mModelMatrix = glm::translate(mModelMatrix, GetPosition() + mPositionOffset);
		glm::vec3 rotation = GetRotation() + GetRotationOffset();
		mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
		mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
		mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
		mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mRadius, mRadius, mRadius));

		mShader->uniform("model", mModelMatrix);

		mShader->uniform("outlineWidth", 1.f);

		mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

		glDisable(GL_DEPTH_TEST);

		mShader->drawOutline(mModel.get());

		glEnable(GL_DEPTH_TEST);
	}
#endif

	bool SphereCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint)
	{
		if (_other == nullptr)
		{
			std::cout << "You should add a collider to an entity with a rigidbody" << std::endl;
			return false;
		}

		// We are sphere, other is box
		std::shared_ptr<BoxCollider> otherBox = std::dynamic_pointer_cast<BoxCollider>(_other);
		if (otherBox)
		{
			glm::vec3 boxCenter = otherBox->GetPosition() + otherBox->mPositionOffset;
			glm::vec3 boxHalfSize = otherBox->GetSize() / 2.0f;
			glm::vec3 sphereCenter = GetPosition() + mPositionOffset;
			float sphereRadius = GetRadius();

			glm::vec3 boxRotation = otherBox->GetRotation() + otherBox->GetRotationOffset();
			glm::mat4 boxRotationMatrix = glm::yawPitchRoll(glm::radians(boxRotation.y), glm::radians(boxRotation.x), glm::radians(boxRotation.z));
			glm::mat3 invBoxRotationMatrix = glm::transpose(glm::mat3(boxRotationMatrix));

			glm::vec3 localSphereCenter = invBoxRotationMatrix * (sphereCenter - boxCenter);

			glm::vec3 closestPoint = glm::clamp(localSphereCenter, -boxHalfSize, boxHalfSize);

			closestPoint = glm::vec3(boxRotationMatrix * glm::vec4(closestPoint, 1.0f)) + boxCenter;

			float distance = glm::length(closestPoint - sphereCenter);

			if (distance <= sphereRadius)
			{
				_collisionPoint = closestPoint;
				return true;
			}
		}

		// We are sphere, other is sphere
		std::shared_ptr<SphereCollider> otherSphere = std::dynamic_pointer_cast<SphereCollider>(_other);
		if (otherSphere)
		{
			glm::vec3 a = GetPosition() + mPositionOffset;
			glm::vec3 b = otherSphere->GetPosition() + otherSphere->GetPositionOffset();
			float ahs = mRadius;
			float bhs = otherSphere->GetRadius();
			float distance = glm::distance(a, b);
			if (distance < ahs + bhs)
			{
				glm::vec3 direction = glm::normalize(b - a);
				_collisionPoint = a + direction * ahs;
				return true;
			}
			return false;
		}

		return false;
	}

}