#include "Suspension.h"

#include "Core.h"
#include "Entity.h"
#include "Transform.h"
#include "Rigidbody.h"

#ifdef _DEBUG
#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

namespace JamesEngine
{

#ifdef _DEBUG
	void Suspension::OnGUI()
	{
        if (!mDebugVisual)
            return;

        // Retrieve the current camera from the entity's core
        std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();

        // Set up the shader uniforms for projection and view matrices
        mShader->uniform("projection", camera->GetProjectionMatrix());
        mShader->uniform("view", camera->GetViewMatrix());

		if (!mWheel || !mAnchorPoint)
			return;

        // Get the anchor and wheel positions for the suspension
        glm::vec3 anchorPos = mAnchorPoint->GetComponent<Transform>()->GetPosition(); 
        glm::vec3 wheelPos = mWheel->GetComponent<Transform>()->GetPosition();

        // Compute the midpoint between the anchor and the wheel.
        glm::vec3 midPoint = (anchorPos + wheelPos) * 0.5f;

        // Compute the direction and length from the anchor to the wheel.
        glm::vec3 suspensionDir = wheelPos - anchorPos;
        float springLength = glm::length(suspensionDir);
        suspensionDir = glm::normalize(suspensionDir);

        // Since the spring model is oriented upward along the y-axis,
        // we need to compute the rotation that aligns (0,1,0) with suspensionDir.
        glm::vec3 up = glm::vec3(0, 1, 0);
        float dotVal = glm::dot(up, suspensionDir);
        glm::vec3 rotationAxis = glm::cross(up, suspensionDir);
        float angle = acos(glm::clamp(dotVal, -1.0f, 1.0f));

        // Handle the case where the vectors are nearly parallel or opposite.
        if (glm::length(rotationAxis) < 1e-6f) {
            if (dotVal < 0.0f) {
                // Vectors are opposite, so choose an arbitrary perpendicular axis.
                rotationAxis = glm::vec3(1, 0, 0);
                angle = glm::pi<float>();
            }
        }
        else {
            rotationAxis = glm::normalize(rotationAxis);
        }

        // Build the model matrix in several stages.
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        // 1. Translate to the midpoint of the suspension.
        modelMatrix = glm::translate(modelMatrix, midPoint);

        // 2. Rotate the spring model so that its local up (0,1,0) is aligned with the suspension direction.
        modelMatrix = modelMatrix * glm::rotate(glm::mat4(1.0f), angle, rotationAxis);

        // 3. Adjust the spring model so that it is centered:
        //    a. Translate it by (0, -0.5, 0) since the model is assumed to span [0,1] in y (its center is at y = 0.5).
        //    b. Scale in y so that the spring spans the full distance between the anchor and the wheel.
        glm::mat4 T_center = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.5f, 0));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, springLength, 1.0f));
        glm::mat4 localTransform = S * T_center;

        // Combine the transformations.
        modelMatrix = modelMatrix * localTransform;

        // Set the final model matrix to the shader.
        mShader->uniform("model", modelMatrix);

        // Additional uniforms for visual debugging (these can be adjusted for your needs).
        mShader->uniform("outlineWidth", 1.0f);
        mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

        // Disable depth testing so the debug visual is fully visible.
        glDisable(GL_DEPTH_TEST);

        // Draw the spring model (assumes mSpringModel holds the proper model).
        mShader->drawOutline(mModel.get());

        // Re-enable depth testing.
        glEnable(GL_DEPTH_TEST);
	}
#endif

    void Suspension::OnTick()
    {
        if (!mWheel || !mCarBody || !mAnchorPoint)
        {
            std::cout << "Suspension component is missing a wheel, car body, or anchor point" << std::endl;
            return;
        }

        if (!mWheelRb || !mCarRb)
        {
            mWheelRb = mWheel->GetComponent<Rigidbody>();
            mCarRb = mCarBody->GetComponent<Rigidbody>();

            if (!mWheelRb || !mCarRb)
            {
                std::cout << "Suspension component is missing a rigidbody on the wheel or car body" << std::endl;
                return;
            }
        }

        // Get positions and the suspension axis (local y of the anchor)
        std::shared_ptr<Transform> anchorTransform = mAnchorPoint->GetComponent<Transform>();
        std::shared_ptr<Transform> wheelTransform = mWheel->GetComponent<Transform>();
        glm::vec3 anchorPos = anchorTransform->GetPosition();
        glm::vec3 wheelPos = wheelTransform->GetPosition();
        glm::vec3 suspensionAxis = glm::normalize(anchorTransform->GetUp());

        // Compute the offset and the component along the suspension axis.
        glm::vec3 offset = wheelPos - anchorPos;
        float currentLength = glm::dot(offset, suspensionAxis);
        // Clamp the suspension length between min and max values
        float clampedLength = glm::clamp(currentLength, mMinLength, mMaxLength);
        float compression = mRestLength - clampedLength;
        //float compression = mRestLength - currentLength;

        // Compute relative velocity along the suspension axis
        glm::vec3 wheelVel = mWheelRb->GetVelocity();
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);
        float relativeVel = glm::dot(wheelVel - carVel, suspensionAxis);

        // Calculate the spring and damper contributions
        float springForceMag = mStiffness * compression;
        float c = 2 * sqrt(mStiffness * mWheelRb->GetMass());
        float damperForceMag = -mDamping * relativeVel;
        glm::vec3 suspensionForce = suspensionAxis * (springForceMag + damperForceMag);

        // Project the wheel position onto the suspension axis line (through anchorPos)
        glm::vec3 projectedPos = anchorPos + suspensionAxis * currentLength;

		float dt = GetCore()->DeltaTime();

        // Apply equal and opposite forces to the wheel and the car body.
        mWheelRb->ApplyForce(suspensionForce, wheelPos);
        mCarRb->ApplyForce(-suspensionForce, anchorPos);

        // Correct the wheel laterally over time to avoid snapping
        float alpha = 0.5f;
        glm::vec3 correctedPos = glm::mix(wheelTransform->GetPosition(), projectedPos, alpha);
        wheelTransform->SetPosition(correctedPos);
	}

}