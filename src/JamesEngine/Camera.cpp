#include "Camera.h"

#include "Entity.h"
#include "Core.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace JamesEngine
{

	glm::mat4 Camera::GetProjectionMatrix()
	{
		int winWidth, winHeight;
		GetEntity()->GetCore()->GetWindow()->GetWindowSize(winWidth, winHeight);
		glm::mat4 projection = glm::perspective(glm::radians(mFov), (float)winWidth / (float)winHeight, mNearClip, mFarClip);

		return projection;
	}

	glm::mat4 Camera::GetViewMatrix()
	{
		glm::mat4 view(1.0f);
		view = glm::translate(view, glm::vec3(GetPosition().x, GetPosition().y, GetPosition().z));
		view = glm::rotate(view, glm::radians(GetRotation().y), glm::vec3(0, 1, 0));
		view = glm::rotate(view, glm::radians(GetRotation().x), glm::vec3(1, 0, 0));
		view = glm::rotate(view, glm::radians(GetRotation().z), glm::vec3(0, 0, 1));
		view = glm::inverse(view);

		return view;
	}

}