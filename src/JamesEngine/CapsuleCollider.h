#pragma once

#include "Collider.h"

#ifdef _DEBUG
#include "Renderer/Model.h"
#endif

namespace JamesEngine
{
    class CapsuleCollider : public Collider
    {
    public:
#ifdef _DEBUG
        void OnGUI();
#endif

        // Checks collision with another collider.
        // _collisionPoint is set to an approximate contact location if a collision is found.
        bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint);

        // Setters and getters for capsule parameters.
        // The capsule is defined by a central line segment (of length mHeight)
        // and a radius (mRadius).
        void SetHeight(float _height) { mHeight = _height; }
        float GetHeight() { return mHeight; }

        void SetRadius(float _radius) { mRadius = _radius; }
        float GetRadius() { return mRadius; }

        // Helper functions to get the endpoints of the capsule in world space.
        // In local space the endpoints are at (0, +mHeight/2, 0) and (0, -mHeight/2, 0).
        glm::vec3 GetEndpointA();
        glm::vec3 GetEndpointB();

    private:
        float mHeight{ 1.0f };
        float mRadius{ 0.5f };

#ifdef _DEBUG
        std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/capsule.obj");
#endif
    };
}
