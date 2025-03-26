#pragma once

#include "Collider.h"

#include <fcl/fcl.h>

#ifdef _DEBUG
#include "Renderer/Model.h"
#endif


namespace JamesEngine
{

	class BoxCollider : public Collider
	{
	public:
#ifdef _DEBUG
		void OnGUI();
#endif
		void OnInitialize() override
		{
			std::shared_ptr<fcl::Boxd> boxShape = std::make_shared<fcl::Boxd>(mSize.x, mSize.y, mSize.z);
			mShape = boxShape;
			InitFCLObject(mShape);
		}

		bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth);

		glm::mat3 UpdateInertiaTensor(float _mass) override;

		fcl::CollisionObjectd* GetFCLObject() override { return mFCLObject.get(); }

		void SetSize(glm::vec3 _size)
		{ 
			mSize = _size; 
			std::shared_ptr<fcl::Boxd> boxShape = std::make_shared<fcl::Boxd>(mSize.x, mSize.y, mSize.z);
			mShape = boxShape;
			InitFCLObject(mShape);
		}
		glm::vec3 GetSize() { return mSize; }

	private:
		glm::vec3 mSize{ 1 };

		std::shared_ptr<fcl::Boxd> mShape;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/cube.obj");
#endif
	};

}