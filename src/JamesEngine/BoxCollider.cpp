#include "BoxCollider.h"

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
	void BoxCollider::OnGUI()
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
		mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mSize.x/2, mSize.y/2, mSize.z/2));

		mShader->uniform("model", mModelMatrix);

		mShader->uniform("outlineWidth", 1.f);

		mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

		glDisable(GL_DEPTH_TEST);

		mShader->drawOutline(mModel.get());

		glEnable(GL_DEPTH_TEST);
	}
#endif

	bool BoxCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint)
	{
		if (_other == nullptr)
		{
			std::cout << "You should add a collider to an entity with a rigidbody" << std::endl;
			return false;
		}

		// We are box, other is box
		std::shared_ptr<BoxCollider> otherBox = std::dynamic_pointer_cast<BoxCollider>(_other);
		if (otherBox)
		{
            glm::vec3 aPos = GetPosition() + mPositionOffset;
            glm::vec3 bPos = otherBox->GetPosition() + otherBox->GetPositionOffset();
            glm::vec3 aSize = GetSize() / 2.0f;
            glm::vec3 bSize = otherBox->GetSize() / 2.0f;

            glm::vec3 aRotation = GetRotation() + mRotationOffset;
            glm::vec3 bRotation = otherBox->GetRotation() + otherBox->GetRotationOffset();

            glm::mat4 aRotationMatrix = glm::yawPitchRoll(glm::radians(aRotation.y), glm::radians(aRotation.x), glm::radians(aRotation.z));
            glm::mat4 bRotationMatrix = glm::yawPitchRoll(glm::radians(bRotation.y), glm::radians(bRotation.x), glm::radians(bRotation.z));

            glm::vec3 aAxes[3] = {
                glm::vec3(aRotationMatrix[0]),
                glm::vec3(aRotationMatrix[1]),
                glm::vec3(aRotationMatrix[2])
            };

            glm::vec3 bAxes[3] = {
                glm::vec3(bRotationMatrix[0]),
                glm::vec3(bRotationMatrix[1]),
                glm::vec3(bRotationMatrix[2])
            };

            glm::vec3 translation = bPos - aPos;
            translation = glm::vec3(glm::dot(translation, aAxes[0]), glm::dot(translation, aAxes[1]), glm::dot(translation, aAxes[2]));

            glm::mat3 rotation;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    rotation[i][j] = glm::dot(aAxes[i], bAxes[j]);
                }
            }

            glm::mat3 absRotation;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    absRotation[i][j] = std::abs(rotation[i][j]) + std::numeric_limits<float>::epsilon();
                }
            }

            for (int i = 0; i < 3; i++)
            {
                float ra = aSize[i];
                float rb = bSize[0] * absRotation[i][0] + bSize[1] * absRotation[i][1] + bSize[2] * absRotation[i][2];
                if (std::abs(translation[i]) > ra + rb) return false;
            }

            for (int i = 0; i < 3; i++)
            {
                float ra = aSize[0] * absRotation[0][i] + aSize[1] * absRotation[1][i] + aSize[2] * absRotation[2][i];
                float rb = bSize[i];
                if (std::abs(translation[0] * rotation[0][i] + translation[1] * rotation[1][i] + translation[2] * rotation[2][i]) > ra + rb)
                    return false;
            }

            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    float ra = aSize[(i + 1) % 3] * absRotation[(i + 2) % 3][j] + aSize[(i + 2) % 3] * absRotation[(i + 1) % 3][j];
                    float rb = bSize[(j + 1) % 3] * absRotation[i][(j + 2) % 3] + bSize[(j + 2) % 3] * absRotation[i][(j + 1) % 3];
                    if (std::abs(translation[(i + 2) % 3] * rotation[(i + 1) % 3][j] - translation[(i + 1) % 3] * rotation[(i + 2) % 3][j]) > ra + rb)
                        return false;
                }
            }

            _collisionPoint = (aPos + bPos) / 2.0f;
            return true;
		}

		// We are box, other is sphere
		std::shared_ptr<SphereCollider> otherSphere = std::dynamic_pointer_cast<SphereCollider>(_other);
		if (otherSphere)
		{
            glm::vec3 boxCenter = GetPosition() + mPositionOffset;
            glm::vec3 boxHalfSize = GetSize() / 2.0f;
            glm::vec3 sphereCenter = otherSphere->GetPosition() + otherSphere->mPositionOffset;
            float sphereRadius = otherSphere->GetRadius();

            glm::vec3 boxRotation = GetRotation() + mRotationOffset;
            glm::mat4 boxRotationMatrix = glm::yawPitchRoll(glm::radians(boxRotation.y), glm::radians(boxRotation.x), glm::radians(boxRotation.z));
            glm::mat3 invBoxRotationMatrix = glm::transpose(glm::mat3(boxRotationMatrix));

            glm::vec3 localSphereCenter = invBoxRotationMatrix * (sphereCenter - boxCenter);

            glm::vec3 closestPoint = glm::clamp(localSphereCenter, -boxHalfSize, boxHalfSize);

            closestPoint = glm::vec3(boxRotationMatrix * glm::vec4(closestPoint, 1.0f)) + boxCenter;

            float distance = glm::length(closestPoint - sphereCenter);

            if (distance <= sphereRadius)
            {
                _collisionPoint = closestPoint;
                return true;
            }
		}

		// We are box, other is model
		std::shared_ptr<ModelCollider> otherModel = std::dynamic_pointer_cast<ModelCollider>(_other);
        if (otherModel)
        {
            // Get the box's world parameters.
            glm::vec3 boxPos = GetPosition() + GetPositionOffset();
            glm::vec3 boxRotation = GetRotation() + GetRotationOffset();
            glm::vec3 boxSize = GetSize();
            glm::vec3 boxHalfSize = boxSize * 0.5f;

            // Build the box's rotation matrix (from Euler angles).
            glm::mat4 boxRotMatrix = glm::mat4(1.0f);
            boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.x), glm::vec3(1, 0, 0));
            boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.y), glm::vec3(0, 1, 0));
            boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.z), glm::vec3(0, 0, 1));
            // Inverse rotation (since the rotation matrix is orthonormal, the inverse is its transpose)
            glm::mat4 invBoxRotMatrix = glm::transpose(boxRotMatrix);

            // Build the model's world transformation matrix.
            glm::vec3 modelPos = otherModel->GetPosition() + otherModel->GetPositionOffset();
            glm::vec3 modelScale = otherModel->GetScale();
            glm::vec3 modelRotation = otherModel->GetRotation() + otherModel->GetRotationOffset();
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelPos);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, modelScale);

            std::vector<Renderer::Model::Face> faces = otherModel->GetTriangles(boxPos, boxRotation, boxSize);
            for (const auto& face : faces)
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

		// We are box, other is capsule
		std::shared_ptr<CapsuleCollider> otherCapsule = std::dynamic_pointer_cast<CapsuleCollider>(_other);
        if (otherCapsule)
        {
            glm::vec3 A = otherCapsule->GetEndpointA();
            glm::vec3 B = otherCapsule->GetEndpointB();
            const int sampleCount = 5;
            bool collisionFound = false;
            glm::vec3 collisionPointSum(0.0f);

            for (int i = 0; i < sampleCount; i++)
            {
                float t = float(i) / float(sampleCount - 1);
                glm::vec3 samplePoint = A + t * (B - A);
                float effRadius = otherCapsule->EffectiveRadius(t, otherCapsule->GetCapRadius(), otherCapsule->GetCylinderRadius());

                // Sphere vs Box test for the sample point with "radius" = effRadius.
                glm::vec3 boxCenter = GetPosition() + GetPositionOffset();
                glm::vec3 boxHalfSize = GetSize() / 2.0f;
                glm::vec3 boxRotation = GetRotation() + GetRotationOffset();
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

		return false;
	}

}