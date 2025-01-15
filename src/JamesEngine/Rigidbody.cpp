#include "Rigidbody.h"

#include "Entity.h"
#include "Core.h"
#include "Component.h"
#include "Transform.h"
#include "BoxCollider.h"

#include <vector>
#include <iostream>

namespace JamesEngine
{

	void Rigidbody::OnTick()
	{
		// Get all box colliders in the scene
		std::vector<std::shared_ptr<BoxCollider>> boxColliders;
		GetEntity()->GetCore()->FindComponents(boxColliders);

		// Iterate through all box colliders to see if we're colliding with any
		for (auto otherBoxCollider : boxColliders)
		{
			// Skip if it is ourself
			if (otherBoxCollider->GetTransform() == GetTransform())
				continue;

			// Check if colliding
			if (otherBoxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>()))
			{
				std::string otherTag = otherBoxCollider->GetEntity()->GetTag();
				
				for (size_t ci = 0; ci < GetEntity()->mComponents.size(); ci++)
				{
					GetEntity()->mComponents.at(ci)->OnCollision(otherTag);
				}

				std::string thisTag = GetEntity()->GetTag();
				for (size_t ci = 0; ci < otherBoxCollider->GetEntity()->mComponents.size(); ci++)
				{
					otherBoxCollider->GetEntity()->mComponents.at(ci)->OnCollision(thisTag);
				}

				// Kludge (*vomit emoji*)
				float amount = 0.001f;
				float step = 0.001f;

				std::shared_ptr<BoxCollider> boxCollider = GetEntity()->GetComponent<BoxCollider>();

				while (true)
				{
					if (!otherBoxCollider->IsColliding(boxCollider))
						break;

					Move(glm::vec3(amount, 0, 0));
					if (!otherBoxCollider->IsColliding(boxCollider))
						break;

					Move(-glm::vec3(amount, 0, 0));
					Move(-glm::vec3(amount, 0, 0));
					if (!otherBoxCollider->IsColliding(boxCollider))
						break;

					Move(glm::vec3(amount, 0, 0));
					Move(glm::vec3(0, 0, amount));
					if (!otherBoxCollider->IsColliding(boxCollider))
						break;

					Move(-glm::vec3(0, 0, amount));
					Move(-glm::vec3(0, 0, amount));
					if (!otherBoxCollider->IsColliding(boxCollider))
						break;

					Move(glm::vec3(0, 0, amount));
					Move(glm::vec3(0, amount, 0));
					if (!otherBoxCollider->IsColliding(boxCollider))
						break;

					Move(-glm::vec3(0, amount, 0));
					Move(-glm::vec3(0, amount, 0));
					if (!otherBoxCollider->IsColliding(boxCollider))
						break;

					Move(glm::vec3(0, amount, 0));
					amount += step;
				}

			}
		}
	}

}