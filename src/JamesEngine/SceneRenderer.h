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

		void AddModel(int _entityId, std::shared_ptr<Model> _model, const glm::mat4& _transform = glm::mat4(1.0f), const ShadowOverride& _shadow = {});

		void SetSSAORadius(float _radius) { mSSAORadius = _radius; }
		void SetSSAOBias(float _bias) { mSSAOBias = _bias; }
		void SetSSAOPower(float _power) { mSSAOPower = _power; }

		void SetAOStrength(float _strength) { mAOStrength = _strength; }
		void SetAOSpecularScale(float _specScale) { mAOSpecScale = _specScale; }
		void SetAOMin(float _min) { mAOMin = _min; }

	private:
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
		float mNormalOffsetScale = 2.f;

		// SSAO settings
		bool mSSAOEnabled = true;
		float mSSAOResultionScale = 0.5f;
		float mSSAORadius = 0.2f;
		float mSSAOBias = 0.06f;
		float mSSAOPower = 1.4f;
		float mAOBlurScale = 1.0f;

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