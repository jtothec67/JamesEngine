#pragma once

#include "Collider.h"

#ifdef JAMES_DEBUG
#include "Renderer/Model.h"
#endif


namespace JamesEngine
{

	class SphereCollider : public Collider
	{
	public:
#ifdef JAMES_DEBUG
		void OnGUI();
#endif

		bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth);
		bool RayCollision(const Ray& _ray, RaycastHit& _outHit) { return false; }

		glm::mat3 UpdateInertiaTensor(float _mass);

		void SetRadius(float _radius) { mRadius = _radius; }
		float GetRadius() { return mRadius; }

	private:
		float mRadius = 0.5f;

#ifdef JAMES_DEBUG
		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/sphere.obj");
#endif
	};

}