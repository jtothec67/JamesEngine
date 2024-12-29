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
		for (auto boxCollider : boxColliders)
		{
			// Skip if it is ourself
			if (boxCollider->GetTransform() == GetTransform())
				continue;

			// Check if colliding
			if (boxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>()))
			{
				// Kludge (*vomit emoji*)
				float amount = 0.1f;
				float step = 0.1f;
				while (true)
				{
					if (!boxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>())) 
						break;

					SetPosition(GetPosition() + glm::vec3(amount, 0, 0));
					if (!boxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>())) 
						break;

					SetPosition(GetPosition() - glm::vec3(amount, 0, 0));
					SetPosition(GetPosition() - glm::vec3(amount, 0, 0));
					if (!boxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>())) 
						break;

					SetPosition(GetPosition() + glm::vec3(amount, 0, 0));
					SetPosition(GetPosition() + glm::vec3(0, 0, amount));
					if (!boxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>())) 
						break;

					SetPosition(GetPosition() - glm::vec3(0, 0, amount));
					SetPosition(GetPosition() - glm::vec3(0, 0, amount));
					if (!boxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>())) 
						break;

					SetPosition(GetPosition() + glm::vec3(0, 0, amount));
					SetPosition(GetPosition() + glm::vec3(0, amount, 0));
					if (!boxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>())) 
						break;

					SetPosition(GetPosition() - glm::vec3(0, amount, 0));
					SetPosition(GetPosition() - glm::vec3(0, amount, 0));
					if (!boxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>())) 
						break;

					SetPosition(GetPosition() + glm::vec3(0, amount, 0));
					amount += step;
				}

			}
		}
	}

}