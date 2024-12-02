#pragma once

#include "Component.h"

namespace JamesEngine
{
	class Model;
	class Texture;
	class Shader;

	class ModelRenderer : public Component
	{
	public:
		ModelRenderer();

		void SetModel(std::shared_ptr<Model> _model) { mModel = _model; }
		void SetTexture(std::shared_ptr<Texture> _texture) { mTexture = _texture; }
		void SetShader(std::shared_ptr<Shader> _shader) { mShader = _shader; }

		void OnInitialize();
		void OnRender();

	private:
		std::shared_ptr<Model> mModel = nullptr;
		std::shared_ptr<Texture> mTexture = nullptr;
		std::shared_ptr<Shader> mShader;
	};

}