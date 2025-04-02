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
		void OnInitialize();
		void OnRender();

		void SetModel(std::shared_ptr<Model> _model) { mModel = _model; }
		void SetTexture(std::shared_ptr<Texture> _texture) { mTexture = _texture; }
		void SetShader(std::shared_ptr<Shader> _shader) { mShader = _shader; }

		void SetSpecularStrength(float _strength) { mSpecularStrength = _strength; }

		void SetPositionOffset(glm::vec3 _offset) { mPositionOffset = _offset; }
		void SetRotationOffset(glm::vec3 _offset) { mRotationOffset = _offset; }

	private:
		std::shared_ptr<Model> mModel = nullptr;
		std::shared_ptr<Texture> mTexture = nullptr;
		std::shared_ptr<Shader> mShader;

		float mSpecularStrength = 1.f;

		glm::vec3 mPositionOffset{ 0 };
		glm::vec3 mRotationOffset{ 0 };
	};

}