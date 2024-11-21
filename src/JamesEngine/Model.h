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
		void OnLoad() { mModel = std::make_shared<Renderer::Model>(GetPath() + ".obj"); }

	private:
		friend class JamesEngine::ModelRenderer;

		std::shared_ptr<Renderer::Model> mModel;
	};

}