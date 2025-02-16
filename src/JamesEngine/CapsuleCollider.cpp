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
        mModelMatrix = glm::scale(mModelMatrix, glm::vec3(1, 1, 1));

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

    bool CapsuleCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint)
    {
        if (_other == nullptr)
        {
            std::cout << "CapsuleCollider: _other collider is null!" << std::endl;
            return false;
        }

		// We are capsule, other is sphere.
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

            float distance = glm::length(closestPoint - sphereCenter);
            if (distance <= (GetRadius() + sphereRadius))
            {
                _collisionPoint = closestPoint;
                return true;
            }
        }

		// We are capsule, other is box.
        std::shared_ptr<BoxCollider> otherBox = std::dynamic_pointer_cast<BoxCollider>(_other);
        if (otherBox)
        {
            glm::vec3 A = GetEndpointA();
            glm::vec3 B = GetEndpointB();
            const int sampleCount = 5; // Increase for higher accuracy.
            bool collisionFound = false;
            glm::vec3 collisionPointSum(0.0f);

            // For each sample point along the capsule’s segment:
            for (int i = 0; i < sampleCount; i++)
            {
                float t = float(i) / float(sampleCount - 1);
                glm::vec3 samplePoint = A + t * (B - A);

                // --- Sphere vs Box test (using capsule radius) ---
                glm::vec3 boxCenter = otherBox->GetPosition() + otherBox->GetPositionOffset();
                glm::vec3 boxHalfSize = otherBox->GetSize() / 2.0f;
                glm::vec3 boxRotation = otherBox->GetRotation() + otherBox->GetRotationOffset();
                glm::mat4 boxRotMatrix = glm::yawPitchRoll(
                    glm::radians(boxRotation.y),
                    glm::radians(boxRotation.x),
                    glm::radians(boxRotation.z)
                );
                glm::mat3 invBoxRotMatrix = glm::transpose(glm::mat3(boxRotMatrix));

                // Transform the sample point into the box’s local space.
                glm::vec3 localSample = invBoxRotMatrix * (samplePoint - boxCenter);
                // Find the closest point on the box to the sample point.
                glm::vec3 closestPointLocal = glm::clamp(localSample, -boxHalfSize, boxHalfSize);
                glm::vec3 closestPoint = glm::vec3(boxRotMatrix * glm::vec4(closestPointLocal, 1.0f)) + boxCenter;
                float distance = glm::length(closestPoint - samplePoint);
                if (distance <= GetRadius())
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

		// We are capsule, other is model.
        std::shared_ptr<ModelCollider> otherModel = std::dynamic_pointer_cast<ModelCollider>(_other);
        if (otherModel)
        {
            glm::vec3 A = GetEndpointA();
            glm::vec3 B = GetEndpointB();
            // For simplicity, we will use the capsule’s center as a reference.
            glm::vec3 capsuleCenter = (A + B) / 2.0f;
            glm::vec3 capsuleRotation = GetRotation() + GetRotationOffset();
            // Build a transformation for the “capsule space” similar to the box collider.
            glm::mat4 capsuleRotMatrix = glm::yawPitchRoll(
                glm::radians(capsuleRotation.y),
                glm::radians(capsuleRotation.x),
                glm::radians(capsuleRotation.z)
            );
            glm::mat4 invCapsuleRotMatrix = glm::transpose(capsuleRotMatrix);

            // Build the model’s world transformation.
            glm::vec3 modelPos = otherModel->GetPosition() + otherModel->GetPositionOffset();
            glm::vec3 modelScale = otherModel->GetScale();
            glm::vec3 modelRotation = otherModel->GetRotation() + otherModel->GetRotationOffset();
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelPos);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, modelScale);

            // Retrieve the model's triangles.
            std::vector<Renderer::Model::Face> faces = otherModel->GetTriangles(capsuleCenter, capsuleRotation, glm::vec3(1));
            for (const auto& face : faces)
            {
                // Transform triangle vertices into world space.
                glm::vec3 a = glm::vec3(modelMatrix * glm::vec4(face.a.position, 1.0f));
                glm::vec3 b = glm::vec3(modelMatrix * glm::vec4(face.b.position, 1.0f));
                glm::vec3 c = glm::vec3(modelMatrix * glm::vec4(face.c.position, 1.0f));

                // For collision we need the distance from the capsule segment (A, B) to the triangle.
                // Here we assume a helper function exists (in MathsHelper.h) such as:
                //    float Maths::DistanceSegmentTriangle(const glm::vec3& segA, const glm::vec3& segB,
                //                                         const glm::vec3& triA, const glm::vec3& triB, const glm::vec3& triC);
                // If that distance is less than mRadius, we consider it a collision.
                float distance = Maths::DistanceSegmentTriangle(A, B, a, b, c);
                if (distance <= GetRadius())
                {
                    _collisionPoint = capsuleCenter;
                    return true;
                }
            }
        }

		// We are capsule, other is capsule.
        std::shared_ptr<CapsuleCollider> otherCapsule = std::dynamic_pointer_cast<CapsuleCollider>(_other);
        if (otherCapsule)
        {
            glm::vec3 A = GetEndpointA();
            glm::vec3 B = GetEndpointB();
            glm::vec3 C = otherCapsule->GetEndpointA();
            glm::vec3 D = otherCapsule->GetEndpointB();

            // Use our Maths helper to get the distance between the two segments.
            float distance = Maths::DistanceSegmentSegment(A, B, C, D);
            if (distance <= (GetRadius() + otherCapsule->GetRadius()))
            {
                // Compute the closest points on each segment.
                glm::vec3 cp1, cp2;
                Maths::ClosestPointsSegmentSegment(A, B, C, D, cp1, cp2);
                // Use the midpoint of the two closest points as the collision point.
                _collisionPoint = (cp1 + cp2) * 0.5f;
                return true;
            }
        }


        return false;
    }
}
