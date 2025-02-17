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

        bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint);

        void SetHeight(float _height) { mHeight = _height; }
        float GetHeight() { return mHeight; }

        void SetCylinderRadius(float _radius) { mCylinderRadius = _radius; }
        float GetCylinderRadius() const { return mCylinderRadius; }

        // Radius used for the hemispherical endcaps.
        void SetCapRadius(float _radius) { mCapRadius = _radius; }
        float GetCapRadius() const { return mCapRadius; }

        glm::vec3 GetEndpointA();
        glm::vec3 GetEndpointB();

        float EffectiveRadius(float t, float capRadius, float cylRadius);

    private:
        float mHeight = 1.0f;
        float mCylinderRadius = 0.5f;
        float mCapRadius = 0.5f;

#ifdef _DEBUG
        std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/capsule.obj");
#endif
    };
}
