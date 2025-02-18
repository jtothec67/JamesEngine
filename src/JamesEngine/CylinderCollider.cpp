#include "CylinderCollider.h"

#include "Core.h"
#include "SphereCollider.h"
#include "ModelCollider.h"
#include "CapsuleCollider.h"
#include "MathsHelper.h"

#include <iostream>

#ifdef _DEBUG
#include "Camera.h"
#include "Entity.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace JamesEngine
{

#ifdef _DEBUG
    void CylinderCollider::OnGUI()
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
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.z + 90), glm::vec3(0, 0, 1));
        mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mRadius / 2, mHeight / 2, mRadius / 2));

        mShader->uniform("model", mModelMatrix);

        mShader->uniform("outlineWidth", 1.f);

        mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

        glDisable(GL_DEPTH_TEST);

        mShader->drawOutline(mModel.get());

        glEnable(GL_DEPTH_TEST);
    }
#endif

    bool CylinderCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint)
    {
        if (_other == nullptr)
        {
            std::cout << "You should add a collider to an entity with a rigidbody" << std::endl;
            return false;
        }

        // We are cylinder, other is model
        std::shared_ptr<ModelCollider> otherModel = std::dynamic_pointer_cast<ModelCollider>(_other);
        if (otherModel)
        {
            // --- 1. Get Cylinder Parameters ---
            // Assume GetPosition(), GetRotation(), mPositionOffset, mRotationOffset are available.
            glm::vec3 cylCenter = GetPosition() + mPositionOffset;
            glm::vec3 cylRotation = GetRotation() + mRotationOffset; // in degrees
            float h = GetHeight();  // length of the center line
            float R = GetRadius();  // cylinder's radius

            // --- 2. Build Cylinder World Transform and Its Inverse ---
            // In local space the cylinder is centered at the origin, aligned along Y
            // with flat faces at y = ±h/2.
            glm::mat4 cylRotMat = glm::yawPitchRoll(glm::radians(cylRotation.y),
                glm::radians(cylRotation.x),
                glm::radians(cylRotation.z));
            glm::mat4 cylTrans = glm::translate(glm::mat4(1.0f), cylCenter);
            glm::mat4 cylWorldMat = cylTrans * cylRotMat;
            glm::mat4 invCylWorldMat = glm::inverse(cylWorldMat);

            // --- 3. Define a Bounding Box for the BVH Query ---
            // In cylinder local space a box of size (2*R, h, 2*R) completely covers the cylinder.
            // We use the cylinder's center and rotation.
            glm::vec3 bboxPos = cylCenter;
            glm::vec3 bboxRot = cylRotation;
            glm::vec3 bboxSize(2 * R, h, 2 * R);

            // Retrieve candidate triangles from the model.
            // (Triangles are returned in model space.)
            std::vector<Renderer::Model::Face> faces = otherModel->GetTriangles(bboxPos, bboxRot, bboxSize);

            // --- 4. Build the Model's World Transform ---
            // Since ModelCollider does not provide a GetModelMatrix(), we construct it manually.
            glm::vec3 modelPos = otherModel->GetPosition() + otherModel->GetPositionOffset();
            glm::vec3 modelRot = otherModel->GetRotation() + otherModel->GetRotationOffset();
            glm::vec3 modelScale = otherModel->GetScale();
            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), modelPos);
            // Apply rotations (assuming X then Y then Z rotations).
            modelMatrix = modelMatrix * glm::yawPitchRoll(glm::radians(modelRot.y),
                glm::radians(modelRot.x),
                glm::radians(modelRot.z));
            modelMatrix = glm::scale(modelMatrix, modelScale);

            // --- 5. Process Triangles and Sample Points ---
            bool collisionDetected = false;
            glm::vec3 contactSumLocal(0.0f); // Accumulate contact points in cylinder local space
            float totalPenetration = 0.0f;

            // For each triangle face...
            for (const auto& face : faces)
            {
                // Convert triangle vertices from model space to world space.
                glm::vec3 A_world = glm::vec3(modelMatrix * glm::vec4(face.a.position, 1.0f));
                glm::vec3 B_world = glm::vec3(modelMatrix * glm::vec4(face.b.position, 1.0f));
                glm::vec3 C_world = glm::vec3(modelMatrix * glm::vec4(face.c.position, 1.0f));

                // Now convert these vertices from world space to cylinder local space.
                glm::vec3 A_local = glm::vec3(invCylWorldMat * glm::vec4(A_world, 1.0f));
                glm::vec3 B_local = glm::vec3(invCylWorldMat * glm::vec4(B_world, 1.0f));
                glm::vec3 C_local = glm::vec3(invCylWorldMat * glm::vec4(C_world, 1.0f));

                // Define a set of sample points on the triangle:
                glm::vec3 samples[7];
                samples[0] = A_local;
                samples[1] = B_local;
                samples[2] = C_local;
                samples[3] = (A_local + B_local) * 0.5f;
                samples[4] = (A_local + C_local) * 0.5f;
                samples[5] = (B_local + C_local) * 0.5f;
                samples[6] = (A_local + B_local + C_local) / 3.0f;

                // Process each sample point.
                for (int i = 0; i < 7; i++)
                {
                    float sdf = CylinderSDF(samples[i], h, R);
                    // If the SDF is negative, the sample is inside (or on) the cylinder.
                    if (sdf <= 0.0f)
                    {
                        collisionDetected = true;
                        float penetration = -sdf; // positive penetration depth
                        // Accumulate the sample (in cylinder local space) weighted by penetration.
                        contactSumLocal += samples[i] * penetration;
                        totalPenetration += penetration;
                    }
                }
            }

            // --- 6. Determine the Collision Contact Point ---
            if (collisionDetected && totalPenetration > 0.0f)
            {
                // Compute the weighted average contact point in cylinder local space.
                glm::vec3 contactLocal = contactSumLocal / totalPenetration;
                // Transform the contact point back into world space.
                glm::vec3 contactWorld = glm::vec3(cylWorldMat * glm::vec4(contactLocal, 1.0f));
                _collisionPoint = contactWorld;
                return true;
            }
        }

        return false;
    }

    float CylinderCollider::CylinderSDF(const glm::vec3& p, float h, float R)
    {
        float halfH = h * 0.5f;
        // Radial distance from the y–axis.
        float radial = sqrt(p.x * p.x + p.z * p.z);
        float d_radial = radial - R;
        // Vertical distance from the flat caps.
        float d_vertical = fabs(p.y) - halfH;
        // Standard capped cylinder SDF.
        float inside = std::min(std::max(d_radial, d_vertical), 0.0f);
        float outside = glm::length(glm::max(glm::vec2(d_radial, d_vertical), glm::vec2(0.0f)));
        return inside + outside;
    }

}