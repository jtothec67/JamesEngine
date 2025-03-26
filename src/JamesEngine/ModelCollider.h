#pragma once

#include "Collider.h"
#include "Model.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace JamesEngine
{

    class ModelCollider : public Collider
    {
    public:
#ifdef _DEBUG
        void OnGUI();
#endif
        bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth);

        glm::mat3 UpdateInertiaTensor(float _mass);

        void SetModel(std::shared_ptr<Model> _model);
        std::shared_ptr<Model> GetModel() { return mModel; }

		void IsConvex(bool _isConvex) { mIsConvex = _isConvex; }
		bool IsConvex() { return mIsConvex; }

    private:
        std::shared_ptr<Model> mModel = nullptr;

        std::shared_ptr<fcl::BVHModel<fcl::OBBRSSd>> mNonConvexShape;
		std::shared_ptr<fcl::Convexd> mConvexShape;

        bool mIsConvex = false;
    };

}
