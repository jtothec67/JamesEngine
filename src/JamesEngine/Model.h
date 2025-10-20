#pragma once

#include "Resource.h"
#include "Renderer/Model.h"

#include <memory>

namespace JamesEngine
{

	class ModelRenderer;

	class Model : public Resource
	{
	public:
		void OnLoad() { mModel = std::make_shared<Renderer::Model>(GetPath()); }

	private:
		friend class SceneRenderer;
		friend class ModelRenderer;
		friend class ModelCollider;
		friend class SphereCollider;
		friend class BoxCollider;

		std::shared_ptr<Renderer::Model> mModel;
	};

}