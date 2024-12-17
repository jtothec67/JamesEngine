#include "Rigidbody.h"

#include "Entity.h"
#include "Core.h"
#include "Component.h"
#include "BoxCollider.h"

#include <vector>
#include <iostream>

namespace JamesEngine
{

	void Rigidbody::OnTick()
	{
		std::vector<std::shared_ptr<BoxCollider>> boxColliders;
		GetEntity()->GetCore()->FindComponents(boxColliders);

		for (auto boxCollider : boxColliders)
		{
			if (boxCollider->GetTransform() == GetTransform())
				continue;

			if (boxCollider->IsColliding(GetEntity()->GetComponent<BoxCollider>()))
			{
				std::cout << "Colliding!" << std::endl;
			}
		}
	}

}