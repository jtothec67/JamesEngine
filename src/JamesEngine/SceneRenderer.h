#pragma once

#include "Model.h"

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

	class SceneRenderer
	{
	public:
		SceneRenderer(std::shared_ptr<Core> _core) { mCore = _core; }
		~SceneRenderer() {}

		void RenderScene();

		void AddModel(std::shared_ptr<Model> _model, const glm::mat4& _transform = glm::mat4(1.0f), const ShadowOverride& _shadow = {});

	private:
		void ClearScene(); // Clears all models from the scene for next frame

		std::vector<SubmissionInfo> mSubmissions;

		std::weak_ptr<Core> mCore;
	};

}