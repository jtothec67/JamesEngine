#include "ModelCollider.h"

#include "Core.h"
#include "SphereCollider.h"
#include "BoxCollider.h"

#include "MathsHelper.h"

#ifdef _DEBUG
#include "Camera.h"
#include "Entity.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

namespace JamesEngine
{

#ifdef _DEBUG
    void ModelCollider::OnGUI()
    {
        if (mModel == nullptr)
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
        mModelMatrix = glm::scale(mModelMatrix, glm::vec3(GetScale().x, GetScale().y, GetScale().z));

        mShader->uniform("model", mModelMatrix);

        mShader->uniform("outlineWidth", 1.f);

        mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

        glDisable(GL_DEPTH_TEST);

        mShader->drawOutline(mModel->mModel.get());

        glEnable(GL_DEPTH_TEST);
    }
#endif

    bool ModelCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint)
    {
        if (_other == nullptr)
        {
            std::cout << "You should add a collider to an entity with a rigidbody" << std::endl;
            return false;
        }

		if (mModel == nullptr)
		{
			std::cout << "You need to add a model to the model collider" << std::endl;
			return false;
		}

		// We are model, other is sphere
        std::shared_ptr<SphereCollider> otherSphere = std::dynamic_pointer_cast<SphereCollider>(_other);
		if (otherSphere)
		{
            // Get sphere world position and radius.
            glm::vec3 spherePos = otherSphere->GetPosition() + otherSphere->GetPositionOffset();
            float sphereRadius = otherSphere->GetRadius();
            float sphereRadiusSq = sphereRadius * sphereRadius;

            // Build the model's world transformation matrix.
            glm::vec3 modelPos = GetPosition() + GetPositionOffset();
            glm::vec3 modelScale = GetScale();
            glm::vec3 modelRotation = GetRotation() + GetRotationOffset();

            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelPos);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, modelScale);

            // Iterate over each triangle face of the model.
            for (const auto& face : mModel->mModel->GetFaces())
            {
                // Transform each vertex into world space.
                glm::vec3 a = glm::vec3(modelMatrix * glm::vec4(face.a.position, 1.0f));
                glm::vec3 b = glm::vec3(modelMatrix * glm::vec4(face.b.position, 1.0f));
                glm::vec3 c = glm::vec3(modelMatrix * glm::vec4(face.c.position, 1.0f));

                // Compute the closest point on this triangle to the sphere center.
                glm::vec3 closestPoint = Maths::ClosestPointOnTriangle(spherePos, a, b, c);

                // Check if the distance from the sphere's center to this point is within the radius.
                glm::vec3 diff = spherePos - closestPoint;
                float distanceSq = glm::dot(diff, diff);
                if (distanceSq <= sphereRadiusSq)
                {
                    _collisionPoint = closestPoint;
                    return true;
                }
            }
		}

		// We are model, other is box
        std::shared_ptr<BoxCollider> otherBox = std::dynamic_pointer_cast<BoxCollider>(_other);
        if (otherBox)
		{
            // Get the box's world parameters.
            glm::vec3 boxPos = otherBox->GetPosition() + otherBox->GetPositionOffset();
            glm::vec3 boxRotation = otherBox->GetRotation() + otherBox->GetRotationOffset();
            glm::vec3 boxSize = otherBox->GetSize();
            glm::vec3 boxHalfSize = boxSize * 0.5f;

            // Build the box's rotation matrix (from Euler angles).
            glm::mat4 boxRotMatrix = glm::mat4(1.0f);
            boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.x), glm::vec3(1, 0, 0));
            boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.y), glm::vec3(0, 1, 0));
            boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.z), glm::vec3(0, 0, 1));
            // Inverse rotation (since the rotation matrix is orthonormal, the inverse is its transpose)
            glm::mat4 invBoxRotMatrix = glm::transpose(boxRotMatrix);

            // Build the model's world transformation matrix.
            glm::vec3 modelPos = GetPosition() + GetPositionOffset();
            glm::vec3 modelScale = GetScale();
            glm::vec3 modelRotation = GetRotation() + GetRotationOffset();
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelPos);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, modelScale);

            // Test each triangle face of the model against the box.
            for (const auto& face : mModel->mModel->GetFaces())
            {
                // Transform triangle vertices into world space.
                glm::vec3 a = glm::vec3(modelMatrix * glm::vec4(face.a.position, 1.0f));
                glm::vec3 b = glm::vec3(modelMatrix * glm::vec4(face.b.position, 1.0f));
                glm::vec3 c = glm::vec3(modelMatrix * glm::vec4(face.c.position, 1.0f));

                // Transform the vertices into the box's local space.
                glm::vec3 aLocal = glm::vec3(invBoxRotMatrix * glm::vec4(a - boxPos, 1.0f));
                glm::vec3 bLocal = glm::vec3(invBoxRotMatrix * glm::vec4(b - boxPos, 1.0f));
                glm::vec3 cLocal = glm::vec3(invBoxRotMatrix * glm::vec4(c - boxPos, 1.0f));
                glm::vec3 triVerts[3] = { aLocal, bLocal, cLocal };

                // Use the SAT-based triangle-box test.
                if (Maths::TriBoxOverlap(triVerts, boxHalfSize))
                {
                    // Compute an approximate collision point.
                    // Here we take the closest point on the triangle (in world space)
                    // to the box center.
                    glm::vec3 closestPoint = Maths::ClosestPointOnTriangle(boxPos, a, b, c);
                    _collisionPoint = closestPoint;
                    return true;
                }
            }
		}

		// We are model, other is model
        // DO NOT DO THIS
        std::shared_ptr<ModelCollider> otherModel = std::dynamic_pointer_cast<ModelCollider>(_other);
        if (otherModel)
        {
			std::cout << "Model to model collision is very slow! Are you sure you meant to do this? You can comment this line out and continue if you did." << std::endl;

            // Build world transform for "this" model.
            glm::vec3 modelPos = GetPosition() + GetPositionOffset();
            glm::vec3 modelScale = GetScale();
            glm::vec3 modelRotation = GetRotation() + GetRotationOffset();
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelPos);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, modelScale);

            // Build world transform for the other model.
            glm::vec3 otherModelPos = otherModel->GetPosition() + otherModel->GetPositionOffset();
            glm::vec3 otherModelScale = otherModel->GetScale();
            glm::vec3 otherModelRotation = otherModel->GetRotation() + otherModel->GetRotationOffset();
            glm::mat4 otherModelMatrix = glm::mat4(1.0f);
            otherModelMatrix = glm::translate(otherModelMatrix, otherModelPos);
            otherModelMatrix = glm::rotate(otherModelMatrix, glm::radians(otherModelRotation.x), glm::vec3(1, 0, 0));
            otherModelMatrix = glm::rotate(otherModelMatrix, glm::radians(otherModelRotation.y), glm::vec3(0, 1, 0));
            otherModelMatrix = glm::rotate(otherModelMatrix, glm::radians(otherModelRotation.z), glm::vec3(0, 0, 1));
            otherModelMatrix = glm::scale(otherModelMatrix, otherModelScale);

            // Get faces for each model.
            const std::vector<Renderer::Model::Face>& facesA = mModel->mModel->GetFaces();
            const std::vector<Renderer::Model::Face>& facesB = otherModel->mModel->mModel->GetFaces();

            // Test every triangle from this model against every triangle from the other model.
            for (const auto& faceA : facesA)
            {
                glm::vec3 A0 = glm::vec3(modelMatrix * glm::vec4(faceA.a.position, 1.0f));
                glm::vec3 A1 = glm::vec3(modelMatrix * glm::vec4(faceA.b.position, 1.0f));
                glm::vec3 A2 = glm::vec3(modelMatrix * glm::vec4(faceA.c.position, 1.0f));

                for (const auto& faceB : facesB)
                {
                    glm::vec3 B0 = glm::vec3(otherModelMatrix * glm::vec4(faceB.a.position, 1.0f));
                    glm::vec3 B1 = glm::vec3(otherModelMatrix * glm::vec4(faceB.b.position, 1.0f));
                    glm::vec3 B2 = glm::vec3(otherModelMatrix * glm::vec4(faceB.c.position, 1.0f));

                    if (Maths::TrianglesIntersect(A0, A1, A2, B0, B1, B2))
                    {
                        // For an approximate collision point, take the midpoint between the triangle centroids.
                        glm::vec3 centroidA = (A0 + A1 + A2) / 3.0f;
                        glm::vec3 centroidB = (B0 + B1 + B2) / 3.0f;
                        _collisionPoint = (centroidA + centroidB) * 0.5f;
                        return true;
                    }
                }
            }
        }

		return false;
    }

}