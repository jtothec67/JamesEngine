#pragma once

#include "Component.h"

#include "Renderer/Shader.h"


namespace JamesEngine
{
	class Model;
	class Texture;

	class ModelRenderer : public Component
	{
	public:
		ModelRenderer();

		void SetModel(std::shared_ptr<Model> _model) { mModel = _model; }
		void SetTexture(std::shared_ptr<Texture> _texture) { mTexture = _texture; }

		void OnRender();

	private:
		std::shared_ptr<Model> mModel = nullptr;
		std::shared_ptr<Texture> mTexture = nullptr;
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../shaders/ObjShader.vert", "../shaders/ObjShader.frag");
	};

}