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
		void OnGUI();
#endif
		void OnInitialize() override
		{
			std::shared_ptr<fcl::Sphered> sphereShape = std::make_shared<fcl::Sphered>(mRadius);
			mShape = sphereShape;
			InitFCLObject(mShape);
		}

		bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth);

		glm::mat3 UpdateInertiaTensor(float _mass);

		fcl::CollisionObjectd* GetFCLObject() override { return mFCLObject.get(); }

		void SetRadius(float _radius)
		{
			mRadius = _radius;
			std::shared_ptr<fcl::Sphered> sphereShape = std::make_shared<fcl::Sphered>(mRadius);
			mShape = sphereShape;
			InitFCLObject(mShape);
		}
		float GetRadius() { return mRadius; }

	private:
		float mRadius = 0.5f;

		std::shared_ptr<fcl::Sphered> mShape;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/sphere.obj");
#endif
	};

}