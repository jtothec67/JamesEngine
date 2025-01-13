#include "Skybox.h"

namespace JamesEngine
{

	void Skybox::RenderSkybox()
	{
		if (!mTexture)
			return;

		mShader->drawSkybox(mMesh.get(),mTexture->mTexture.get());
	}
}