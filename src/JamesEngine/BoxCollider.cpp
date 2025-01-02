#include "BoxCollider.h"
#include "Core.h"

#ifdef _DEBUG

#include "Camera.h"
#include "Entity.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#endif // _DEBUG


#include <iostream>

namespace JamesEngine
{

#ifdef _DEBUG
	void BoxCollider::OnRender()
	{
		std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();

		mShader->uniform("projection", camera->GetProjectionMatrix());

		mShader->uniform("view", camera->GetViewMatrix());

		glm::mat4 mModelMatrix = glm::mat4(1.f);
		mModelMatrix = glm::translate(mModelMatrix, GetPosition() + mOffset);
		mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mSize.x/2, mSize.y/2, mSize.z/2));

		mShader->uniform("model", mModelMatrix);

		mShader->uniform("outlineWidth", 1.f);

		mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

		glDisable(GL_DEPTH_TEST);

		mShader->drawOutline(mModel.get());

		glEnable(GL_DEPTH_TEST);
	}
#endif // _DEBUG

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