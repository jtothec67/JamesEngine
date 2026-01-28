#pragma once

#include "Model.h"
#include "Shader.h"

namespace JamesEngine
{

	class Core;

	enum class ShadowMode
	{
		None,
		Proxy,
		Default
	};

	struct ShadowOverride
	{
		ShadowMode mode = ShadowMode::Default;
		std::shared_ptr<Model> proxy = nullptr; // Used only if mode == Proxy
	};

	struct MaterialRenderInfo
	{
		Renderer::Model::MaterialGroup& materialGroup;
		std::shared_ptr<Model> model; // Reference to the model to get the embedded textures
		glm::mat4 transform; // Model transform

		uint64_t occlusionKey = 0; // Unique key for occlusion queries
	};

	struct OcclusionInfo
	{
		GLuint queryIds[2];
		bool visible = true;
		bool hasResult = false;

		uint64_t lastFrameTested = 0;
		uint64_t lastFrameSubmitted = 0;
	};

	class SceneRenderer
	{
	public:
		SceneRenderer(std::shared_ptr<Core> _core);
		~SceneRenderer() {}

		void RenderScene();

		// User toggleable settings
		// SSAO
		void EnableSSAO(bool _enabled) {
			mSSAOEnabled = _enabled;
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_UseSSAO", mSSAOEnabled);
		}
		bool IsSSAOEnabled() const { return mSSAOEnabled; }
		void SetSSAORadius(float _radius) {
			mSSAORadius = _radius;
			mSSAOShader->mShader->use();
			mSSAOShader->mShader->uniform("u_Radius", mSSAORadius);
		}
		void SetSSAOBias(float _bias) { 
			mSSAOBias = _bias;
			mSSAOShader->mShader->use();
			mSSAOShader->mShader->uniform("u_Bias", mSSAOBias);
		}
		void SetSSAOPower(float _power) { 
			mSSAOPower = _power;
			mSSAOShader->mShader->use();
			mSSAOShader->mShader->uniform("u_Power", mSSAOPower);
		}
		void SetSSAOBlurScale(float _blurScale) { mSSAOBlurScale = _blurScale; }

		// AO application
		void SetAOStrength(float _strength) {
			mAOStrength = _strength;
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_AOStrength", mAOStrength);
		}
		void SetAOSpecularScale(float _specScale) {
			mAOSpecScale = _specScale;
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_AOSpecScale", mAOSpecScale);
		}
		void SetAOMin(float _min) {
			mAOMin = _min;
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_AOMin", mAOMin);
		}

		// Bloom
		void EnableBloom(bool _enabled) {
			mBloomEnabled = _enabled;
			mCompositeShader->mShader->use();
			mCompositeShader->mShader->uniform("u_UseBloom", mBloomEnabled);
		}
		bool IsBloomEnabled() const { return mBloomEnabled; }
		void SetBloomThreshold(float _threshold) {
			mBloomThreshold = _threshold;
			mBrightPassShader->mShader->use();
			mBrightPassShader->mShader->uniform("u_BloomThreshold", mBloomThreshold);
		}
		void SetBloomStrength(float _strength) {
			mBloomStrength = _strength;
			mCompositeShader->mShader->use();
			mCompositeShader->mShader->uniform("u_BloomStrength", mBloomStrength);
		}
		void SetBloomKnee(float _knee) {
			mBloomKnee = _knee;
			mBrightPassShader->mShader->use();
			mBrightPassShader->mShader->uniform("u_BloomKnee", mBloomKnee);
		}
		void SetBloomLevels(int _levels);
		void SetBloomMinSize(int _minSize) {
			mBloomMinSize = _minSize;
			SetBloomLevels(mBloomLevels); // Could make sure user sets bloom min size befoore setting bloom levels, but whatever
		}

		// Shadows
		void SetSoftShadowBase(float _base) {
			mPCSSBase = _base;
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_PCSSBase", mPCSSBase);
		}
		void SetSoftShadowScale(float _scale) {
			mPCSSScale = _scale;
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_PCSSScale", mPCSSScale);
		}
		void SetShadowBiasSlope(float _slope) {
			mShadowBiasSlope = _slope;
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_ShadowBiasSlope", mShadowBiasSlope);
		}
		void SetShadowBiasMin(float _min) {
			mShadowBiasMin = _min;
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_ShadowBiasMin", mShadowBiasMin);
		}
		void SetShadowNormalOffsetScale(float _scale) {
			mShadowNormalOffsetScale = _scale;
			mObjShader->mShader->use();
			mObjShader->mShader->uniform("u_NormalOffsetScale", mShadowNormalOffsetScale);
		}

		// Tone mapping
		void SetExposure(float _exposure) {
			mExposure = _exposure;
			mToneMapShader->mShader->use();
			mToneMapShader->mShader->uniform("u_Exposure", mExposure);
		}

	private:
		friend class ModelRenderer;

		void AddModel(int _entityId, std::shared_ptr<Model> _model, const glm::mat4& _transform = glm::mat4(1.0f), const ShadowOverride& _shadow = {});

		void ClearScene(); // Clears all models from the scene for next frame

		std::vector<MaterialRenderInfo> FrustumCulledMaterials(
			const std::vector<MaterialRenderInfo>& _materials,
			const glm::mat4& _view,
			const glm::mat4& _proj);

		enum class DepthSortMode
		{
			FrontToBack,   // opaque
			BackToFront    // transparent
		};

		std::vector<MaterialRenderInfo> FrustumCulledDistanceSortedMaterials(
			const std::vector<MaterialRenderInfo>& _materials,
			const glm::mat4& _view,
			const glm::mat4& _proj,
			const glm::vec3& _posWS,
			DepthSortMode _mode);

		std::weak_ptr<Core> mCore;

		std::vector<MaterialRenderInfo> mOpaqueMaterials;
		std::vector<MaterialRenderInfo> mTransparentMaterials;
		std::vector<MaterialRenderInfo> mShadowMaterials;

		std::unordered_map<uint64_t, OcclusionInfo> mOcclusionCache;

		// Fallback PBR values
		glm::vec4 mBaseColorStrength{ 1.f };
		float mMetallicness = 0.0f;
		float mRoughness = 1.0f;
		float mAOFallback = 1.0f;
		glm::vec3 mEmmisive{ 0.0f };

		// Shaders
		std::shared_ptr<Shader> mObjShader;
		std::shared_ptr<Shader> mDepthAlphaShader;
		std::shared_ptr<Shader> mDepthShader;
		std::shared_ptr<Shader> mSSAOShader;
		std::shared_ptr<Shader> mScalarBlurShader;
		std::shared_ptr<Shader> mVec3BlurShader;
		std::shared_ptr<Shader> mBrightPassShader;
		std::shared_ptr<Shader> mDownsample2x;
		std::shared_ptr<Shader> mUpsampleAdd;
		std::shared_ptr<Shader> mCompositeShader;
		std::shared_ptr<Shader> mToneMapShader;
		std::shared_ptr<Shader> mOcclusionBoxShader;

		// Textures
		std::shared_ptr<Renderer::RenderTexture> mShadingPass;
		std::shared_ptr<Renderer::RenderTexture> mDepthPass;
		std::shared_ptr<Renderer::RenderTexture> mAORaw;
		std::shared_ptr<Renderer::RenderTexture> mAOIntermediate;
		std::shared_ptr<Renderer::RenderTexture> mAOBlurred;
		std::shared_ptr<Renderer::RenderTexture> mBrightPassScene;
		std::vector<std::shared_ptr<Renderer::RenderTexture>> mBloomDown;
		std::vector<std::shared_ptr<Renderer::RenderTexture>> mBloomTemp;
		std::vector<std::shared_ptr<Renderer::RenderTexture>> mBloomBlur;
		std::shared_ptr<Renderer::RenderTexture> mBloomIntermediate;
		std::shared_ptr<Renderer::RenderTexture> mBloom;
		std::shared_ptr<Renderer::RenderTexture> mCompositeScene;

		// Quad mesh for full-screen passes
		std::shared_ptr<Renderer::Mesh> mRect = std::make_shared<Renderer::Mesh>();

		// Cube mesh for occlusion queries
		std::shared_ptr<Renderer::Mesh> mCube = std::make_shared<Renderer::Mesh>();

		// Bloom settings
		bool mBloomEnabled = true;
		float mBloomThreshold = 1.3f;
		float mBloomStrength = 1.f;
		float mBloomKnee = 0.65f;
		int mBloomLevels = 5; // Number of downsample/blur levels
		int mBloomMinSize = 16; // Minimum resolution of the smallest bloom level

		// Shadow settings
		float mPCSSBase = 1.f;
		float mPCSSScale = 10.f;

		float mShadowBiasSlope = 0.0022;
		float mShadowBiasMin = 0.0002;
		float mShadowNormalOffsetScale = 2.f;

		// SSAO settings
		bool mSSAOEnabled = true;
		float mSSAOResultionScale = 0.5f;
		float mSSAORadius = 0.2f;
		float mSSAOBias = 0.06f;
		float mSSAOPower = 1.4f;
		float mSSAOBlurScale = 1.0f;

		// AO settings
		float mAOStrength = 1.0f;
		float mAOSpecScale = 1.0f;
		float mAOMin = 0.05f;

		// Tone mapping settings
		float mExposure = 1.f;

		uint64_t mFrameIndex = 0;

		// Misc
		glm::ivec2 mLastViewportSize{ 1,1 };
	};

}