#include "RayCollider.h"

#include "Core.h"

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
    void RayCollider::OnGUI()
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

    bool RayCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth)
    {
        if (_other == nullptr)
        {
            std::cout << "You should add a collider to an entity with a rigidbody" << std::endl;
            return false;
        }

        //// --- Compute the capsule transform for the ray ---
        //// Get the entity's world-space position and rotation.
        //// mPosition and mDirection are in local space.
        //glm::vec3 worldOrigin = GetPosition() + mPosition;
        //glm::vec3 worldDirection = glm::normalize(GetRotation() * mDirection);

        //// The capsule should span the ray's maximum distance (mHeight).
        //// Its center is halfway along the ray.
        //glm::vec3 capsuleCenter = worldOrigin + worldDirection * (mHeight * 0.5f);

        //// fcl::Capsuled is defined along the z-axis by default.
        //// Compute the rotation needed to align the default z-axis (0,0,1) with the ray direction.
        //glm::vec3 defaultDir(0.0f, 0.0f, 1.0f);
        //glm::quat capsuleRotation = glm::rotation(defaultDir, worldDirection);
        //capsuleRotation = glm::normalize(capsuleRotation);

        //// Convert the capsule rotation and translation into an FCL transform.
        //Eigen::Quaterniond eigenQuat(capsuleRotation.w, capsuleRotation.x, capsuleRotation.y, capsuleRotation.z);
        //fcl::Matrix3d rotMatrix = eigenQuat.toRotationMatrix();
        //fcl::Vector3d translation(capsuleCenter.x, capsuleCenter.y, capsuleCenter.z);
        //fcl::Transform3d capsuleTransform;
        //capsuleTransform.linear() = rotMatrix;
        //capsuleTransform.translation() = translation;

        //// Update the FCL collision object transform for this RayCollider.
        //mFCLObject->setTransform(capsuleTransform);
        //mFCLObject->computeAABB();

        glm::vec3 worldOrigin = GetPosition() + (GetQuaternion() * mPosition);
        glm::vec3 worldDirection = glm::normalize(GetQuaternion() * mDirection);

        // Center of the capsule is halfway along the ray.
        glm::vec3 translation = worldOrigin + worldDirection * (mHeight * 0.5f);

        glm::quat rotation;
        // Compute rotation: default capsule is aligned with (0,0,1)
        glm::vec3 defaultDir(0, 0, 1);
        float dotVal = glm::dot(defaultDir, worldDirection);
        if (dotVal < -0.9999f)
        {
            rotation = glm::angleAxis(glm::pi<float>(), glm::vec3(1, 0, 0));
        }
        else
        {
            rotation = glm::rotation(defaultDir, worldDirection);
        }
        rotation = glm::normalize(rotation);

		UpdateFCLTransform(translation, rotation);

        // Update the other collider's transform as well.
        _other->UpdateFCLTransform(_other->GetPosition(), _other->GetRotation());

        // --- Perform collision detection ---
        fcl::CollisionRequestd request;
        request.enable_contact = true;
        request.num_max_contacts = 10;

        fcl::CollisionResultd result;
        fcl::collide(mFCLObject.get(), _other->GetFCLObject(), request, result);

        if (result.isCollision())
        {
            std::vector<fcl::Contactd> contacts;
            result.getContacts(contacts);

            if (!contacts.empty())
            {
				std::cout << "Num of collisions: " << contacts.size() << std::endl;
                // Find the contact that lies closest to the ray origin.
                float bestT = std::numeric_limits<float>::max();
                size_t bestIndex = 0;
                bool found = false;

                for (size_t i = 0; i < contacts.size(); i++)
                {
                    glm::vec3 contactPos(contacts[i].pos[0],
                        contacts[i].pos[1],
                        contacts[i].pos[2]);
                    // Compute t: projection of (contactPos - worldOrigin) onto the ray direction.
                    float tContact = glm::dot(contactPos - worldOrigin, worldDirection);

                    // Ensure t is within the ray's extent.
                    if (tContact >= 0.0f && tContact <= mHeight && tContact < bestT)
                    {
                        bestT = tContact;
                        bestIndex = i;
                        found = true;
                    }
                }

                if (found)
                {
                    // Use the best contact.
                    const fcl::Contactd& contact = contacts[bestIndex];
                    // Compute the collision point along the ray:
                    // Collision point = worldOrigin + t * worldDirection.
                    _collisionPoint = worldOrigin + bestT * worldDirection;
                    // Use the contact normal.
                    _normal = glm::vec3(contact.normal[0], contact.normal[1], contact.normal[2]);
                    // Compute penetration depth as how far back from the ray's far end this contact is.
                    // (This is an approximation; adjust as needed.)
                    _penetrationDepth = mHeight - bestT;

					std::cout << "Collision point: " << _collisionPoint.x << ", " << _collisionPoint.y << ", " << _collisionPoint.z << std::endl;
					std::cout << "Penetration depth: " << _penetrationDepth << std::endl;

                    return true;
                }
            }
        }
        return false;
    }

}