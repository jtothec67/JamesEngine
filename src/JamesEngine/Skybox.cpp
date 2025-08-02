#include "Skybox.h"

#include "Core.h"
#include "Camera.h"

#include <glm/glm.hpp>

namespace JamesEngine
{

	Skybox::Skybox(std::shared_ptr<Core> _core)
	{
		mCore = _core;
	}

	void Skybox::RenderSkybox()
	{
		if (!mTexture)
			return;
		std::shared_ptr<Camera> camera = mCore.lock()->GetCamera();
		glm::mat4 projection = camera->GetProjectionMatrix();
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 invPV = glm::inverse(projection * view);

		mShader->uniform("u_InvPV", invPV);

		mShader->drawSkybox(mMesh.get(),mTexture->mTexture.get());
	}
}