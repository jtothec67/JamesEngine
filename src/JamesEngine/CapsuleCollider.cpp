#include "CapsuleCollider.h"

#include "BoxCollider.h"
#include "SphereCollider.h"
#include "ModelCollider.h"
#include "MathsHelper.h" // Assume this provides helper functions such as ClosestPointOnSegment().
#include "Core.h"

#include <iostream>
#include <limits>

#ifdef _DEBUG
#include "Camera.h"
#include "Entity.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

namespace JamesEngine
{

#ifdef _DEBUG
    void CapsuleCollider::OnGUI()
    {
        if (!mDebugVisual)
            return;

        std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();

        mShader->uniform("projection", camera->GetProjectionMatrix());

        mShader->uniform("view", camera->GetViewMatrix());

        glm::mat4 mModelMatrix = glm::mat4(1.f);
        mModelMatrix = glm::translate(mModelMatrix, GetPosition() + mPositionOffset);
        glm::vec3 rotation = GetRotation() + GetRotationOffset();
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mCylinderRadius * 2.f, mHeight, mCylinderRadius * 2.f));

        mShader->uniform("model", mModelMatrix);

        mShader->uniform("outlineWidth", 1.f);

        mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

        glDisable(GL_DEPTH_TEST);

        mShader->drawOutline(mModel.get());

        glEnable(GL_DEPTH_TEST);
    }
#endif

    // Returns the world-space position of the “top” endpoint.
    // (Local endpoint at (0, +mHeight/2, 0))
    glm::vec3 CapsuleCollider::GetEndpointA()
    {
        // Start with the local point.
        glm::vec3 localPoint(0, mHeight / 2.0f, 0);
        // Build the capsule’s rotation matrix.
        glm::vec3 capsuleRotation = GetRotation() + GetRotationOffset();
        glm::mat4 rotMatrix = glm::yawPitchRoll(
            glm::radians(capsuleRotation.y),
            glm::radians(capsuleRotation.x),
            glm::radians(capsuleRotation.z)
        );
        // Transform to world space.
        return glm::vec3(rotMatrix * glm::vec4(localPoint, 1.0f)) + GetPosition() + GetPositionOffset();
    }

    // Returns the world-space position of the “bottom” endpoint.
    // (Local endpoint at (0, -mHeight/2, 0))
    glm::vec3 CapsuleCollider::GetEndpointB()
    {
        glm::vec3 localPoint(0, -mHeight / 2.0f, 0);
        glm::vec3 capsuleRotation = GetRotation() + GetRotationOffset();
        glm::mat4 rotMatrix = glm::yawPitchRoll(
            glm::radians(capsuleRotation.y),
            glm::radians(capsuleRotation.x),
            glm::radians(capsuleRotation.z)
        );
        return glm::vec3(rotMatrix * glm::vec4(localPoint, 1.0f)) + GetPosition() + GetPositionOffset();
    }

    // Returns the effective radius at a given parameter t along the capsule’s spine.
    // For t near 0 or 1, we use mCapRadius; otherwise, mCylinderRadius.
    float CapsuleCollider::EffectiveRadius(float t, float capRadius, float cylRadius)
    {
        const float epsilon = 0.001f;
        if (t <= epsilon || t >= (1.0f - epsilon))
            return capRadius;
        return cylRadius;
    }

    bool CapsuleCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint)
    {
        if (_other == nullptr)
        {
            std::cout << "CapsuleCollider: _other collider is null!" << std::endl;
            return false;
        }

        // --- Capsule vs Sphere ---
        std::shared_ptr<SphereCollider> otherSphere = std::dynamic_pointer_cast<SphereCollider>(_other);
        if (otherSphere)
        {
            glm::vec3 sphereCenter = otherSphere->GetPosition() + otherSphere->GetPositionOffset();
            float sphereRadius = otherSphere->GetRadius();
            glm::vec3 A = GetEndpointA();
            glm::vec3 B = GetEndpointB();
            glm::vec3 AB = B - A;
            float t = glm::dot(sphereCenter - A, AB) / glm::dot(AB, AB);
            t = glm::clamp(t, 0.0f, 1.0f);
            glm::vec3 closestPoint = A + t * AB;
            float effRadius = EffectiveRadius(t, mCapRadius, mCylinderRadius);
            if (glm::length(sphereCenter - closestPoint) <= (effRadius + sphereRadius))
            {
                _collisionPoint = closestPoint;
                return true;
            }
        }

        // --- Capsule vs Box ---
        std::shared_ptr<BoxCollider> otherBox = std::dynamic_pointer_cast<BoxCollider>(_other);
        if (otherBox)
        {
            glm::vec3 A = GetEndpointA();
            glm::vec3 B = GetEndpointB();
            const int sampleCount = 5;
            bool collisionFound = false;
            glm::vec3 collisionPointSum(0.0f);

            for (int i = 0; i < sampleCount; i++)
            {
                float t = float(i) / float(sampleCount - 1);
                glm::vec3 samplePoint = A + t * (B - A);
                float effRadius = EffectiveRadius(t, mCapRadius, mCylinderRadius);

                // Sphere vs Box test for the sample point with "radius" = effRadius.
                glm::vec3 boxCenter = otherBox->GetPosition() + otherBox->GetPositionOffset();
                glm::vec3 boxHalfSize = otherBox->GetSize() / 2.0f;
                glm::vec3 boxRotation = otherBox->GetRotation() + otherBox->GetRotationOffset();
                glm::mat4 boxRotMatrix = glm::yawPitchRoll(
                    glm::radians(boxRotation.y),
                    glm::radians(boxRotation.x),
                    glm::radians(boxRotation.z)
                );
                glm::mat3 invBoxRotMatrix = glm::transpose(glm::mat3(boxRotMatrix));

                glm::vec3 localSample = invBoxRotMatrix * (samplePoint - boxCenter);
                glm::vec3 closestPointLocal = glm::clamp(localSample, -boxHalfSize, boxHalfSize);
                glm::vec3 closestPoint = glm::vec3(boxRotMatrix * glm::vec4(closestPointLocal, 1.0f)) + boxCenter;

                if (glm::length(samplePoint - closestPoint) <= effRadius)
                {
                    collisionFound = true;
                    collisionPointSum += closestPoint;
                }
            }
            if (collisionFound)
            {
                _collisionPoint = collisionPointSum / float(sampleCount);
                return true;
            }
        }

        // --- Capsule vs Model ---
        std::shared_ptr<ModelCollider> otherModel = std::dynamic_pointer_cast<ModelCollider>(_other);
        if (otherModel)
        {
            glm::vec3 A = GetEndpointA();
            glm::vec3 B = GetEndpointB();
            const int sampleCount = 5;
            bool collisionFound = false;
            glm::vec3 collisionPointSum(0.0f);

            // Build the model's world transformation.
            glm::vec3 modelPos = otherModel->GetPosition() + otherModel->GetPositionOffset();
            glm::vec3 modelScale = otherModel->GetScale();
            glm::vec3 modelRotation = otherModel->GetRotation() + otherModel->GetRotationOffset();
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelPos);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, modelScale);

            // Retrieve model triangles.
            glm::vec3 capsuleCenter = (A + B) * 0.5f;
            glm::vec3 capsuleRotation = GetRotation() + GetRotationOffset();
            glm::vec3 capsuleSize = glm::vec3(GetCylinderRadius() * 2, GetHeight() + (2 * GetCapRadius()), GetCylinderRadius() * 2);
            std::vector<Renderer::Model::Face> faces = otherModel->GetTriangles(capsuleCenter, capsuleRotation, capsuleSize);

            for (int i = 0; i < sampleCount; i++)
            {
                float t = float(i) / float(sampleCount - 1);
                glm::vec3 samplePoint = A + t * (B - A);
                float effRadius = EffectiveRadius(t, mCapRadius, mCylinderRadius);

                for (const auto& face : faces)
                {
                    // Transform triangle vertices into world space.
                    glm::vec3 a = glm::vec3(modelMatrix * glm::vec4(face.a.position, 1.0f));
                    glm::vec3 b = glm::vec3(modelMatrix * glm::vec4(face.b.position, 1.0f));
                    glm::vec3 c = glm::vec3(modelMatrix * glm::vec4(face.c.position, 1.0f));

                    // Use the Maths helper to compute the distance from the sample point to the triangle.
                    float d = Maths::DistancePointTriangle(samplePoint, a, b, c);
                    if (d <= effRadius)
                    {
                        collisionFound = true;
                        collisionPointSum += samplePoint; // Alternatively, use a computed closest point.
                    }
                }
            }
            if (collisionFound)
            {
                _collisionPoint = collisionPointSum / float(sampleCount);
                return true;
            }
        }

        // --- Capsule vs Capsule ---
        std::shared_ptr<CapsuleCollider> otherCapsule = std::dynamic_pointer_cast<CapsuleCollider>(_other);
        if (otherCapsule)
        {
            glm::vec3 A = GetEndpointA();
            glm::vec3 B = GetEndpointB();
            glm::vec3 C = otherCapsule->GetEndpointA();
            glm::vec3 D = otherCapsule->GetEndpointB();
            const int sampleCount = 5;
            bool collisionFound = false;
            glm::vec3 collisionPointSum(0.0f);
            const float eps = 0.001f;

            for (int i = 0; i < sampleCount; i++)
            {
                float t1 = float(i) / float(sampleCount - 1);
                glm::vec3 sample1 = A + t1 * (B - A);
                float effRadius1 = EffectiveRadius(t1, mCapRadius, mCylinderRadius);
                for (int j = 0; j < sampleCount; j++)
                {
                    float t2 = float(j) / float(sampleCount - 1);
                    glm::vec3 sample2 = C + t2 * (D - C);
                    float effRadius2 = EffectiveRadius(t2, otherCapsule->GetCapRadius(), otherCapsule->GetCylinderRadius());
                    if (glm::length(sample1 - sample2) <= (effRadius1 + effRadius2))
                    {
                        collisionFound = true;
                        collisionPointSum += (sample1 + sample2) * 0.5f;
                    }
                }
            }
            if (collisionFound)
            {
                _collisionPoint = collisionPointSum / float(sampleCount * sampleCount);
                return true;
            }
        }

        return false;
    }
}
