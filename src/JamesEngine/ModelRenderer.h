#pragma once

#include "Component.h"

#include <vector>

namespace JamesEngine
{
	class Model;
	class Texture;
	class Shader;

	class ModelRenderer : public Component
	{
	public:
		void OnInitialize();
		void OnAlive();
		void OnRender();
		void OnShadowRender(const glm::mat4& _lightSpaceMatrix);

		void SetModel(std::shared_ptr<Model> _model) { mModel = _model; }
		void AddTexture(std::shared_ptr<Texture> _texture) { mTextures.push_back(_texture); }
		void SetShader(std::shared_ptr<Shader> _shader) { mShader = _shader; }

		void SetSpecularStrength(float _strength) { mSpecularStrength = _strength; }

		void SetPositionOffset(glm::vec3 _offset) { mPositionOffset = _offset; }
		glm::vec3 GetPositionOffset() { return mPositionOffset; }

		void SetRotationOffset(glm::vec3 _offset) { mRotationOffset = _offset; }
		glm::vec3 GetRotationOffset() { return mRotationOffset; }

        void SetAlphaCutoff(float _cutoff) { mAlphaCutoff = glm::clamp(_cutoff, 0.f, 1.f); }
		float GetAlphaCutoff() { return mAlphaCutoff; }

		void SetPreBakeShadows(bool _preBake) { mPreBakeShadows = _preBake; }
		bool GetPreBakeShadows() { return mPreBakeShadows; }

	private:
		std::shared_ptr<Model> mModel = nullptr;
		std::shared_ptr<Shader> mShader = nullptr;

		std::vector<std::shared_ptr<Texture>> mTextures;

		float mSpecularStrength = 1.f;

		glm::vec3 mPositionOffset{ 0 };
		glm::vec3 mRotationOffset{ 0 };

		std::shared_ptr<Shader> mDepthShader;

		float mAlphaCutoff = 0.5f;

		bool mPreBakeShadows = false;
		glm::vec3 mCustomCenter{ 0, -65, 0 }; // Used for pre-baked shadows, if the model is not centered at the origin
		glm::ivec2 mCustomShadowMapSize{ 2000, 1500 }; // Custom shadow map size for pre-baked shadows
	};

}