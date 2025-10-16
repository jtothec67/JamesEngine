#include "SceneRenderer.h"

#include "Core.h"

namespace JamesEngine
{

	void SceneRenderer::RenderScene()
	{
		//ScopedTimer timer("SceneRenderer::RenderScene");

		// Do rendering

		/*for (int i = 0; i < mSubmissions.size(); ++i)
		{
			std::cout << "Rendering model " << i << std::endl;

			if (mSubmissions[i].shadowMode == ShadowMode::Proxy)
				std::cout << "This model uses a proxy shadow model." << std::endl;
		}*/

		ClearScene();
	}

	void SceneRenderer::AddModel(std::shared_ptr<Model> _model, const glm::mat4& _transform, const ShadowOverride& _shadow)
	{
		SubmissionInfo info;
		info.model = _model;
		info.transform = _transform;
		info.shadowMode = _shadow.mode;
		info.shadowModel = _shadow.proxy;

		if (info.shadowMode == ShadowMode::Proxy && !info.shadowModel)
			info.shadowMode = ShadowMode::Default; // User said to use proxy but didn't provide one, fallback to default

		mSubmissions.push_back(info);
	}

	void SceneRenderer::ClearScene()
	{
		mSubmissions.clear();
	}

}