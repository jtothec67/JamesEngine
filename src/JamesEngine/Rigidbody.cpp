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
				std::string otherTag = otherCollider->GetEntity()->GetTag();
				
				for (size_t ci = 0; ci < GetEntity()->mComponents.size(); ci++)
				{
					GetEntity()->mComponents.at(ci)->OnCollision(otherCollider->GetEntity());
				}

				std::string thisTag = GetEntity()->GetTag();
				for (size_t ci = 0; ci < otherCollider->GetEntity()->mComponents.size(); ci++)
				{
					otherCollider->GetEntity()->mComponents.at(ci)->OnCollision(GetEntity());
				}

				// Kludge (*vomit emoji*)
				float amount = 0.01f;
				float step = 0.1f;

				while (true)
				{
					if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
						break;

					Move(glm::vec3(amount, 0, 0));
					if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
						break;

					Move(-glm::vec3(amount, 0, 0));
					Move(-glm::vec3(amount, 0, 0));
					if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
						break;

					Move(glm::vec3(amount, 0, 0));
					Move(glm::vec3(0, 0, amount));
					if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
						break;

					Move(-glm::vec3(0, 0, amount));
					Move(-glm::vec3(0, 0, amount));
					if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
						break;

					Move(glm::vec3(0, 0, amount));
					Move(glm::vec3(0, amount, 0));
					if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
						break;

					Move(-glm::vec3(0, amount, 0));
					Move(-glm::vec3(0, amount, 0));
					if (!otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
						break;

					Move(glm::vec3(0, amount, 0));
					amount += step;
				}
			}
		}
	}

}