#pragma once 

#include "Collider.h"

#ifdef _DEBUG
#include "Renderer/Model.h"
#endif


namespace JamesEngine
{

	class CylinderCollider : public Collider
	{
	public:
#ifdef _DEBUG
		void OnGUI();
#endif

		bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint);

		void SetHeight(float _height) { mHeight = _height; }
		float GetHeight() { return mHeight; }

		void SetRadius(float _radius) { mRadius = _radius; }
		float GetRadius() { return mRadius; }

		float CylinderCollider::CylinderSDF(const glm::vec3& p, float h, float R);

	private:
		float mHeight = 1.f;
		float mRadius = 1.f;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/cylinder.obj");
#endif
	};

}