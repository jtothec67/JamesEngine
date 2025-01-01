#include "Component.h"

#include <glm/glm.hpp>

namespace JamesEngine
{

	class Camera : public Component
	{
	public:
		glm::mat4 GetViewMatrix();
		glm::mat4 GetProjectionMatrix();

		void SetFov(float fov) { mFov = fov; }
		void SetNearClip(float nearClip) { mNearClip = nearClip; }
		void SetFarClip(float farClip) { mFarClip = farClip; }

		void SetPriority(float priority) { mPriority = priority; }
		float GetPriority() { return mPriority; }

	private:
		float mFov = 60.f;
		float mNearClip = 0.1f;
		float mFarClip = 100.f;

		// Higher priotity, more important
		float mPriority = 1.f;
	};

}