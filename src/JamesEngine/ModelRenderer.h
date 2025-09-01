#pragma once

#include "Component.h"

#include "Renderer/Texture.h"

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

		void SetShadowModel(std::shared_ptr<Model> _model) { mShadowModel = _model; }

		void SetSpecularStrength(float _strength) { mSpecularStrength = _strength; }

		void SetBaseColorStrength(glm::vec4 _strength) { mBaseColorStrength = _strength; }
		void SetMetallicness(float _metallicness) { mMetallicness = glm::clamp(_metallicness, 0.f, 1.f); }
		void SetRoughness(float _roughness) { mRoughness = glm::clamp(_roughness, 0.f, 1.f); }
		void SetAOStrength(float _aoStrength) { mAOStrength = glm::clamp(_aoStrength, 0.f, 1.f); }
		void SetEmmisive(glm::vec3 _emmisive) { mEmmisive = _emmisive; }

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

		std::vector<std::shared_ptr<Texture>> mTextures; // Legacy (haven't got rid yet), used for .obj models
		std::vector<Renderer::Texture*> mRawTextures; // Optimisation assumes no new textures added at runtime (after OnAlive)

		std::shared_ptr<Model> mShadowModel = nullptr; // Optional simpler model for shadow rendering 

		float mSpecularStrength = 1.f;
		
		glm::vec4 mBaseColorStrength{ 1.f };
		float mMetallicness = 0.0f;
		float mRoughness = 1.0f;
		float mAOStrength = 1.0f;
		glm::vec3 mEmmisive{ 0.0f };

		glm::vec3 mPositionOffset{ 0 };
		glm::vec3 mRotationOffset{ 0 };

		std::shared_ptr<Shader> mDepthShader;

		float mAlphaCutoff = 0.5f;

		bool mPreBakeShadows = false;
		glm::vec3 mCustomCenter{ 0, -65, 0 }; // Used for pre-baked shadows, if the model is not centered at the origin
		glm::ivec2 mCustomShadowMapSize{ 2000, 1500 }; // Custom shadow map size for pre-baked shadows
		bool mSplitPrebakedShadowMap = true;
	};

}