#pragma once

#include "Collider.h"

#include <fcl/fcl.h>

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
		void OnInitialize() override
		{
			std::shared_ptr<fcl::Cylinderd> cylinderShape = std::make_shared<fcl::Cylinderd>(mRadius, mHeight);
			mShape = cylinderShape;
			InitFCLObject(mShape);
		}

		bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth);

		glm::mat3 UpdateInertiaTensor(float _mass) override;

		fcl::CollisionObjectd* GetFCLObject() override { return mFCLObject.get(); }

		void SetSize(float _radius, float _height)
		{
			mRadius = _radius;
			mHeight = _height;
			std::shared_ptr<fcl::Cylinderd> cylinderShape = std::make_shared<fcl::Cylinderd>(mRadius, mHeight);
			mShape = cylinderShape;
			InitFCLObject(mShape);
		}
		float GetRadius() { return mRadius; }
		float GetHeight() { return mHeight; }

	private:
		float mRadius = 1.0f;
		float mHeight = 2.0f;

		std::shared_ptr<fcl::Cylinderd> mShape;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/cylinder.obj");
#endif
	};

}