#pragma once

#include "Renderer/Mesh.h"
#include "Renderer/Shader.h"

#include <memory>

namespace JamesEngine
{

	class Texture;
	class Core;

	class GUI
	{
	public:
        GUI(std::shared_ptr<Core> _core);
		~GUI() {}

		int Button(glm::vec2 _position, glm::vec2 _size, std::shared_ptr<Texture> _texture);
		void test() {}

	private:
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../assets/shaders/GUIShader.vert", "../assets/shaders/GUIShader.frag");
		std::shared_ptr<Renderer::Mesh> mRect = std::make_shared<Renderer::Mesh>();

		std::weak_ptr<Core> mCore;
	};

}