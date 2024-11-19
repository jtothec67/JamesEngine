#include "Texture.h"

namespace JamesEngine
{

	void Texture::OnLoad()
	{
		mTexture = std::make_shared<Renderer::Texture>(GetPath());
	}

}