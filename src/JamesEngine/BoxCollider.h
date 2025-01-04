#pragma once

#include "Component.h"

#ifdef _DEBUG

#include "Renderer/Model.h"
#include "Renderer/Shader.h"

#endif


namespace JamesEngine
{

	class BoxCollider : public Component
	{
	public:
#ifdef _DEBUG
		void OnRender();
#endif

		bool IsColliding(std::shared_ptr<BoxCollider> _other);

		void SetSize(glm::vec3 _size) { mSize = _size; }
		void SetOffset(glm::vec3 _offset) { mOffset = _offset; }

	private:
		glm::vec3 mSize{ 1 };
		glm::vec3 mOffset{ 0 };

#ifdef _DEBUG

		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/cube.obj");
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../assets/shaders/OutlineShader.vert","../assets/shaders/OutlineShader.frag");

#endif
	};

}