#pragma once

#include "Resource.h"
#include "Renderer/Texture.h"

#include <memory>

namespace JamesEngine
{

	class Texture : Resource
	{
	public:
		void OnLoad();

	private:
		std::shared_ptr<Renderer::Texture> mTexture;
	};

}