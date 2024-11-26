#pragma once

#include "Resource.h"
#include "Renderer/Mesh.h"

#include <memory>

namespace JamesEngine
{

	class ModelRenderer;

	class Mesh : public Resource
	{
	public:
		void OnLoad() { mMesh = std::make_shared<Renderer::Mesh>(GetPath() + ".png"); }

	private:
		friend class ModelRenderer;

		std::shared_ptr<Renderer::Mesh> mMesh;
	};

}