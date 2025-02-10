#pragma once

#include "Component.h"

#ifdef _DEBUG
#include "Renderer/Shader.h"
#endif


namespace JamesEngine
{

	class Collider : public Component
	{
	public:
#ifdef _DEBUG
		virtual void OnGUI() {}
#endif

		virtual bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint) = 0;

		void SetPositionOffset(glm::vec3 _offset) { mPositionOffset = _offset; }
		glm::vec3 GetPositionOffset() { return mPositionOffset; }

		void SetRotationOffset(glm::vec3 _rotation) { mRotationOffset = _rotation; }
		glm::vec3 GetRotationOffset() { return mRotationOffset; }

		void SetDebugVisual(bool _value) { mDebugVisual = _value; }

	protected:
		friend class BoxCollider;
		friend class SphereCollider;

		glm::vec3 mPositionOffset{ 0 };
		glm::vec3 mRotationOffset{ 0 };

		bool mDebugVisual = true;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../assets/shaders/OutlineShader.vert", "../assets/shaders/OutlineShader.frag");
#endif
	};

}