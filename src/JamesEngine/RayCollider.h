#pragma once

#include "Collider.h"

#include <fcl/fcl.h>

#ifdef _DEBUG
#include "Renderer/Model.h"
#endif


namespace JamesEngine
{

	class RayCollider : public Collider
	{
	public:
#ifdef _DEBUG
		void OnGUI();
#endif
		void OnInitialize() override
		{
			std::shared_ptr<fcl::Capsuled> capsuleShape = std::make_shared<fcl::Capsuled>(mRadius, mHeight);
			mShape = capsuleShape;
			InitFCLObject(mShape);
		}

		bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth);

		glm::mat3 UpdateInertiaTensor(float _mass) { return glm::mat3(1); }

		fcl::CollisionObjectd* GetFCLObject() override { return mFCLObject.get(); }

		void SetMaxDistance(float _maxDistance) { mHeight = _maxDistance; UpdateShape(); }

	private:
		void UpdateShape()
		{
			std::shared_ptr<fcl::Capsuled> capsuleShape = std::make_shared<fcl::Capsuled>(mRadius, mHeight);
			mShape = capsuleShape;
			InitFCLObject(mShape);
		}

		glm::vec3 mPosition = glm::vec3(0);
		glm::vec3 mDirection = glm::vec3(0, -1, 0);

		const float mRadius = 0.0001f;
		float mHeight = 5.0f;

		std::shared_ptr<fcl::Capsuled> mShape;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/cylinder.obj");
#endif
	};

}