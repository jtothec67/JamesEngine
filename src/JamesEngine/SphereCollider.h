#pragma once

#include "Collider.h"

#ifdef _DEBUG
#include "Renderer/Model.h"
#endif


namespace JamesEngine
{

	class SphereCollider : public Collider
	{
	public:
#ifdef _DEBUG
		void OnRender();
#endif

		bool IsColliding(std::shared_ptr<Collider> _other);

		void SetRadius(float _radius) { mRadius = _radius; }
		float GetRadius() { return mRadius; }

	private:
		float mRadius = 1.f;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/sphere.obj");
#endif
	};

}