#include "Component.h"

#include <glm/glm.hpp>

namespace JamesEngine
{

	class Camera : public Component
	{
	public:
		glm::mat4 GetViewMatrix();
		glm::mat4 GetProjectionMatrix();

	private:
		float mFov = 60.f;
		float mNearClip = 0.1f;
		float mFarClip = 100.f;
	};

}