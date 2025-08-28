#include "Component.h"

#include <glm/glm.hpp>

namespace JamesEngine
{

	enum class CameraType
	{
		Perspective,
		Orthographic
	};

	class Camera : public Component
	{
	public:
		glm::mat4 GetViewMatrix();
		glm::mat4 GetProjectionMatrix();

		void SetCameraType(CameraType _type) { mType = _type; }

		void SetNearClip(float _nearClip) { mNearClip = _nearClip; }
		void SetFarClip(float _farClip) { mFarClip = _farClip; }

		float GetNearClip() { return mNearClip; }
		float GetFarClip() { return mFarClip; }

		void SetFov(float _fov) { mFov = _fov; }

		void SetOrthographicSize(glm::vec2 _size) { mOrthographicSize = _size; }

		void SetPriority(float _priority) { mPriority = _priority; }
		float GetPriority() { return mPriority; }

	private:
		CameraType mType = CameraType::Perspective;

		float mNearClip = 0.3f;
		float mFarClip = 1500.f;

		float mFov = 60.f;

		glm::vec2 mOrthographicSize = glm::vec2(10.f, 10.f);

		// Higher priotity, more important
		float mPriority = 1.f;
	};

}