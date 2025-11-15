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

	struct SubmissionInfo
	{
		std::shared_ptr<Model> model;
		glm::mat4 transform;

		ShadowMode shadowMode = ShadowMode::Default;
		std::shared_ptr<Model> shadowModel = nullptr;
	};

	struct MaterialRenderInfo
	{
		Renderer::Model::MaterialGroup& materialGroup;
		std::shared_ptr<Model> model; // Reference to the model to get the embedded textures
		glm::mat4 transform; // Model transform
	};

	class SceneRenderer
	{
	public:
		SceneRenderer(std::shared_ptr<Core> _core);
		~SceneRenderer() {}

		void RenderScene();

		void AddModel(std::shared_ptr<Model> _model, const glm::mat4& _transform = glm::mat4(1.0f), const ShadowOverride& _shadow = {});

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

		std::vector<SubmissionInfo> mSubmissions;

		std::weak_ptr<Core> mCore;

		std::vector<MaterialRenderInfo> mOpaqueMaterials;
		std::vector<MaterialRenderInfo> mTransparentMaterials;
		std::vector<MaterialRenderInfo> mShadowMaterials;

		bool mDoDepthPrePass = true;

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
		std::shared_ptr<Shader> mBlurShader;

		// Textures
		std::shared_ptr<Renderer::RenderTexture> mShadingPass;
		std::shared_ptr<Renderer::RenderTexture> mDepthPass;
		std::shared_ptr<Renderer::RenderTexture> mAORaw;
		std::shared_ptr<Renderer::RenderTexture> mAOIntermediate;
		std::shared_ptr<Renderer::RenderTexture> mAOBlurred;

		// Quad mesh for full-screen passes
		std::shared_ptr<Renderer::Mesh> mRect = std::make_shared<Renderer::Mesh>();

		// Shadow settings
		float mPCSSBase = 1.f;
		float mPCSSScale = 10.f;

		float mShadowBiasSlope = 0.0022;
		float mShadowBiasMin = 0.0002;
		float mNormalOffsetScale = 2.f;

		// SSAO settings
		float mSSAOResultionScale = 0.5f;
		float mSSAORadius = 0.2f;
		float mSSAOBias = 0.06f;
		float mSSAOPower = 1.4f;

		// AO settings
		float mAOStrength = 1.0f;
		float mAOSpecScale = 1.0f;
		float mAOMin = 0.05f;

		// Misc
		glm::ivec2 mLastViewportSize{ 1,1 };
	};

}