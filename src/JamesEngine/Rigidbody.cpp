#include "Rigidbody.h"

#include "Entity.h"
#include "Core.h"
#include "Component.h"
#include "Transform.h"
#include "Collider.h"
#include "BoxCollider.h"

#include <vector>
#include <iostream>

namespace JamesEngine
{

	void Rigidbody::OnTick()
	{
		// Step 1: Compute each of the forces acting on the object (only gravity by default)
		glm::vec3 force = mMass * mAcceleration;
		AddForce(force);

		// Step 2: Compute collisions
		// Get all colliders in the scene
		std::vector<std::shared_ptr<Collider>> colliders;
		GetEntity()->GetCore()->FindComponents(colliders);

		// Iterate through all colliders to see if we're colliding with any
		for (auto otherCollider : colliders)
		{
			// Skip if it is ourself
			if (otherCollider->GetTransform() == GetTransform())
				continue;

			std::shared_ptr<Collider> collider = GetEntity()->GetComponent<Collider>();

			glm::vec3 collisionPoint;
			glm::vec3 collisionNormal;
			float penetrationDepth;

			// Check if colliding
			if (otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
			{
				// Call OnCOllision for all components on both entities
				for (size_t ci = 0; ci < GetEntity()->mComponents.size(); ci++)
				{
					GetEntity()->mComponents.at(ci)->OnCollision(otherCollider->GetEntity());
				}

				for (size_t ci = 0; ci < otherCollider->GetEntity()->mComponents.size(); ci++)
				{
					otherCollider->GetEntity()->mComponents.at(ci)->OnCollision(GetEntity());
				}

				bool otherIsRigidbody = false;
				std::shared_ptr<Rigidbody> otherRigidbody = std::dynamic_pointer_cast<Rigidbody>(otherCollider);

				if (otherRigidbody)
					otherIsRigidbody = true;



				// Move the object out of the collision


				//// Kludge (*vomit emoji*)
				//float amount = 0.01f;
				//float step = 0.1f;

				//while (true)
				//{
				//	if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
				//		break;

				//	Move(glm::vec3(amount, 0, 0));
				//	if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
				//		break;

				//	Move(-glm::vec3(amount, 0, 0));
				//	Move(-glm::vec3(amount, 0, 0));
				//	if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
				//		break;

				//	Move(glm::vec3(amount, 0, 0));
				//	Move(glm::vec3(0, 0, amount));
				//	if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
				//		break;

				//	Move(-glm::vec3(0, 0, amount));
				//	Move(-glm::vec3(0, 0, amount));
				//	if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
				//		break;

				//	Move(glm::vec3(0, 0, amount));
				//	Move(glm::vec3(0, amount, 0));
				//	if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
				//		break;

				//	Move(-glm::vec3(0, amount, 0));
				//	Move(-glm::vec3(0, amount, 0));
				//	if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
				//		break;

				//	Move(glm::vec3(0, amount, 0));
				//	amount += step;
				//}


			}
		}
	}

	void Rigidbody::UpdateInertiaTensor()
	{
		// Calculate the inertia tensor
		glm::mat3 inertiaTensor = glm::mat3(0);
		glm::vec3 size = GetEntity()->GetComponent<BoxCollider>()->GetSize();
		float x = size.x;
		float y = size.y;
		float z = size.z;

		inertiaTensor[0][0] = mMass / 12 * (y * y + z * z);
		inertiaTensor[1][1] = mMass / 12 * (x * x + z * z);
		inertiaTensor[2][2] = mMass / 12 * (x * x + y * y);

		mInertiaTensorInverse = glm::inverse(inertiaTensor);
	}

}