#include "SphereCollider.h"

#include "Core.h"
#include "BoxCollider.h"
#include "ModelCollider.h"
#include "MathsHelper.h"

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
		if (!mDebugVisual)
			return;

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

	bool SphereCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth)
	{
		if (_other == nullptr)
		{
			std::cout << "You should add a collider to an entity with a rigidbody" << std::endl;
			return false;
		}

		UpdateFCLTransform(GetPosition(), GetRotation());
		_other->UpdateFCLTransform(_other->GetPosition(), _other->GetRotation());

		// Get FCL collision objects
		fcl::CollisionObjectd* objA = GetFCLObject();
		fcl::CollisionObjectd* objB = _other->GetFCLObject();

		// Request contact information
		fcl::CollisionRequestd request;
		request.enable_contact = true;
		request.num_max_contacts = 1;

		fcl::CollisionResultd result;

		// Perform collision
		fcl::collide(objA, objB, request, result);

		if (result.isCollision())
		{
			// Extract contact info
			std::vector<fcl::Contactd> contacts;
			result.getContacts(contacts);

			if (!contacts.empty())
			{
				const fcl::Contactd& contact = contacts[0];

				_collisionPoint = glm::vec3(contact.pos[0], contact.pos[1], contact.pos[2]);
				_normal = glm::vec3(contact.normal[0], contact.normal[1], contact.normal[2]);
				_penetrationDepth = std::fabs(static_cast<float>(contact.penetration_depth));

				return true;
			}
		}
		else
		{
			return false;
		}
	}

	glm::mat3 SphereCollider::UpdateInertiaTensor(float _mass)
	{
		return glm::mat3((2.0f / 5.0f) * _mass * mRadius * mRadius);
	}

}