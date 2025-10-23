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

	private:
		void ClearScene(); // Clears all models from the scene for next frame

		std::vector<SubmissionInfo> mSubmissions;

		std::weak_ptr<Core> mCore;

		std::vector<MaterialRenderInfo> mOpaqueMaterials;
		std::vector<MaterialRenderInfo> mTransparentMaterials;

		// Fallback PBR values
		glm::vec4 mBaseColorStrength{ 1.f };
		float mMetallicness = 0.0f;
		float mRoughness = 1.0f;
		float mAOStrength = 1.0f;
		glm::vec3 mEmmisive{ 0.0f };

		// Shaders
		std::shared_ptr<Shader> mDepthAlphaShader;
		std::shared_ptr<Shader> mDepthShader;
		std::shared_ptr<Shader> mObjShader;
	};

}