#pragma once

#include "Component.h"

#include "Renderer/Model.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"

namespace JamesEngine
{

	class ModelRenderer : public Component
	{
	public:
		ModelRenderer(std::string _modelPath, std::string _modelTexture);
		void OnRender();

	private:
		std::shared_ptr<Renderer::Model> mModel;
		std::shared_ptr<Renderer::Texture> mTexture;
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../shaders/ObjShader.vert", "../shaders/ObjShader.frag");
	};

}