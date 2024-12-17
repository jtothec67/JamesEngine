#include "BoxCollider.h"

#include <iostream>

namespace JamesEngine
{

	bool BoxCollider::IsColliding(std::shared_ptr<BoxCollider> _other)
	{
		glm::vec3 a = GetPosition() + mOffset;
		glm::vec3 b = _other->GetPosition() + _other->mOffset;
		glm::vec3 ahs = mSize / 2.0f;
		glm::vec3 bhs = _other->mSize / 2.0f;

		if (a.x > b.x)
		{
			if (b.x + bhs.x < a.x - ahs.x)
			{
				return false;
			}
		}
		else
		{
			if (a.x + ahs.x < b.x - bhs.x)
			{
				return false;
			}
		}

		if (a.y > b.y)
		{
			if (b.y + bhs.y < a.y - ahs.y)
			{
				return false;
			}
		}
		else
		{
			if (a.y + ahs.y < b.y - bhs.y)
			{
				return false;
			}
		}

		if (a.z > b.z)
		{
			if (b.z + bhs.z < a.z - ahs.z)
			{
				return false;
			}
		}
		else
		{
			if (a.z + ahs.z < b.z - bhs.z)
			{
				return false;
			}
		}

		return true;
	}

}