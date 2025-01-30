#include "SphereCollider.h"

#include "Core.h"
#include "BoxCollider.h"

#ifdef _DEBUG
#include "Camera.h"
#include "Entity.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

#include <iostream>

namespace JamesEngine
{

#ifdef _DEBUG
	void SphereCollider::OnRender()
	{
		std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();

		mShader->uniform("projection", camera->GetProjectionMatrix());

		mShader->uniform("view", camera->GetViewMatrix());

		glm::mat4 mModelMatrix = glm::mat4(1.f);
		mModelMatrix = glm::translate(mModelMatrix, GetPosition() + mOffset);
		mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mRadius, mRadius, mRadius));

		mShader->uniform("model", mModelMatrix);

		mShader->uniform("outlineWidth", 1.f);

		mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

		glDisable(GL_DEPTH_TEST);

		mShader->drawOutline(mModel.get());

		glEnable(GL_DEPTH_TEST);
	}
#endif

	bool SphereCollider::IsColliding(std::shared_ptr<Collider> _other)
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
			glm::vec3 boxCenter = otherBox->GetPosition() + otherBox->mOffset;
			glm::vec3 boxHalfSize = otherBox->GetSize() / 2.0f;
			glm::vec3 sphereCenter = GetPosition() + mOffset;
			float sphereRadius = GetRadius();

			// Calculate the closest point on the box to the sphere center
			glm::vec3 closestPoint = glm::clamp(sphereCenter, boxCenter - boxHalfSize, boxCenter + boxHalfSize);

			// Calculate the distance between the closest point and the sphere center
			float distance = glm::length(closestPoint - sphereCenter);

			// Check if the distance is less than or equal to the sphere's radius
			return distance <= sphereRadius;
		}

		// We are sphere, other is sphere
		std::shared_ptr<SphereCollider> otherSphere = std::dynamic_pointer_cast<SphereCollider>(_other);
		if (otherSphere)
		{
			glm::vec3 a = GetPosition() + mOffset;
			glm::vec3 b = otherSphere->GetPosition() + otherSphere->GetOffset();
			float ahs = mRadius;
			float bhs = otherSphere->GetRadius();
			if (glm::distance(a, b) < ahs + bhs)
			{
				return true;
			}
			return false;
		}

		return false;
	}

}