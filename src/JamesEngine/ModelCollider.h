#pragma once

#include "Collider.h"
#include "Model.h"

namespace JamesEngine
{

	class ModelCollider : public Collider
	{
	public:
#ifdef _DEBUG
		void OnGUI();
#endif

		bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint);

		void SetModel(std::shared_ptr<Model> _model) { mModel = _model; }
		std::shared_ptr<Model> GetModel() { return mModel; }

	private:
		std::shared_ptr<Model> mModel = nullptr;
	};

}