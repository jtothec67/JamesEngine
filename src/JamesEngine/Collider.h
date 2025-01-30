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
		virtual void OnRender() {}
#endif

		virtual bool IsColliding(std::shared_ptr<Collider> _other) = 0;

		void SetOffset(glm::vec3 _offset) { mOffset = _offset; }
		glm::vec3 GetOffset() { return mOffset; }

	private:
		friend class BoxCollider;
		friend class SphereCollider;

		glm::vec3 mOffset{ 0 };

#ifdef _DEBUG
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../assets/shaders/OutlineShader.vert", "../assets/shaders/OutlineShader.frag");
#endif
	};

}