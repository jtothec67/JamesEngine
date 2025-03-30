#include "CylinderCollider.h"

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

    bool CylinderCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth)
    {
        if (_other == nullptr)
        {
            std::cout << "You should add a collider to an entity with a rigidbody" << std::endl;
            return false;
        }

        UpdateFCLTransform(GetPosition(), GetRotation());
        _other->UpdateFCLTransform(_other->GetPosition(), _other->GetRotation());


        // Get FCL collision objects
        fcl::CollisionObjectd* objA = GetFCLObject();
        fcl::CollisionObjectd* objB = _other->GetFCLObject();

        // Request contact information
        fcl::CollisionRequestd request;
        request.enable_contact = true;
        request.num_max_contacts = 1;

        fcl::CollisionResultd result;

        // Perform collision
        fcl::collide(objA, objB, request, result);

        if (result.isCollision())
        {
            // Extract contact info
            std::vector<fcl::Contactd> contacts;
            result.getContacts(contacts);

            if (!contacts.empty())
            {
                const fcl::Contactd& contact = contacts[0];

                _collisionPoint = glm::vec3(contact.pos[0], contact.pos[1], contact.pos[2]);
                _normal = glm::vec3(contact.normal[0], contact.normal[1], contact.normal[2]);
                _penetrationDepth = std::fabs(static_cast<float>(contact.penetration_depth));

                return true;
            }
        }
        else
        {
            return false;
        }
    }

    glm::mat3 CylinderCollider::UpdateInertiaTensor(float _mass)
    {
        float Ixx = (1.0f / 12.0f) * _mass * (3 * mRadius * mRadius + mHeight * mHeight);
        float Iyy = Ixx;
        float Izz = (1.0f / 2.0f) * _mass * mRadius * mRadius;

        return glm::mat3(
            Ixx, 0, 0,
            0, Iyy, 0,
            0, 0, Izz
        );
    }

}