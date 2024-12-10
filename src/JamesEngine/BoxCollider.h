#pragma once

#include "Component.h"

namespace JamesEngine
{

	class BoxCollider : Component
	{
	public:
		bool IsColliding(const BoxCollider& _other);

		void SetSize(glm::vec3 _size) { mSize = _size; }
		void SetOffset(glm::vec3 _offset) { mOffset = _offset; }

	private:
		glm::vec3 mSize{ 1 };
		glm::vec3 mOffset{ 0 };
	};

}