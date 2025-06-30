#include "Camera.h"

#include "Entity.h"
#include "Core.h"
#include "Transform.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace JamesEngine
{

	glm::mat4 Camera::GetProjectionMatrix()
	{
		int winWidth, winHeight;
		GetEntity()->GetCore()->GetWindow()->GetWindowSize(winWidth, winHeight);
		float aspect = (float)winWidth / (float)winHeight;

		if (mType == CameraType::Perspective)
		{
			return glm::perspective(glm::radians(mFov), aspect, mNearClip, mFarClip);
		}
		else
		{
			return glm::ortho(
				-mOrthographicSize.x, mOrthographicSize.x,
				-mOrthographicSize.y, mOrthographicSize.y,
				mNearClip, mFarClip
			);
		}
	}

	glm::mat4 Camera::GetViewMatrix()
	{
		std::shared_ptr<Transform> transform = GetEntity()->GetComponent<Transform>();

		glm::quat orientation = transform->GetWorldRotation();
		glm::vec3 position = transform->GetPosition();

		glm::mat4 rotation = glm::mat4_cast(glm::conjugate(orientation));
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), -position);

		return rotation * translation;
	}

}