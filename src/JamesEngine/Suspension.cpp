#include "Suspension.h"

#include "Core.h"
#include "Entity.h"
#include "Transform.h"
#include "Rigidbody.h"

#ifdef _DEBUG
#include "Camera.h"
#include "Entity.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

namespace JamesEngine
{

#ifdef _DEBUG
	void Suspension::OnGUI()
	{
		
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
        auto anchorTransform = mAnchorPoint->GetComponent<Transform>();
        auto wheelTransform = mWheel->GetComponent<Transform>();
        glm::vec3 anchorPos = anchorTransform->GetPosition();
        glm::vec3 wheelPos = wheelTransform->GetPosition();
        glm::vec3 suspensionAxis = glm::normalize(anchorTransform->GetUp());

        // --- Spring/Damper along the suspension axis ---
        // Compute the offset and the component along the suspension axis.
        glm::vec3 offset = wheelPos - anchorPos;
        float currentLength = glm::dot(offset, suspensionAxis);
        // Clamp the suspension length between min and max values
        float clampedLength = glm::clamp(currentLength, mMinLength, mMaxLength);
        float compression = mRestLength - clampedLength;

        // Compute relative velocity along the suspension axis
        glm::vec3 wheelVel = mWheelRb->GetVelocity();
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);
        float relativeVel = glm::dot(wheelVel - carVel, suspensionAxis);

        // Calculate the spring and damper contributions
        float springForceMag = mStiffness * compression;
        float damperForceMag = mDamping * relativeVel;
        glm::vec3 suspensionForce = suspensionAxis * (springForceMag - damperForceMag);

        // --- Lateral Correction to Lock Movement to the Y Axis ---
        // Project the wheel position onto the suspension axis line (through anchorPos)
        glm::vec3 projectedPos = anchorPos + suspensionAxis * currentLength;
        // The error is any deviation from the projection (i.e. lateral offset)
        glm::vec3 lateralError = wheelPos - projectedPos;
        // Remove any component of the wheel's velocity that is along the suspension axis
        glm::vec3 lateralVel = wheelVel - suspensionAxis * glm::dot(wheelVel, suspensionAxis);

        // Use stronger stiffness and damping to quickly correct lateral motion.
        const float lateralStiffness = mStiffness * 10;
        const float lateralDamping = mDamping * 1;
        glm::vec3 lateralForce = -lateralStiffness * lateralError - lateralDamping * lateralVel;

        // --- Total Force and Application ---
        // Combine the suspension (spring/damper) force with the lateral correction force.
        glm::vec3 totalForce = suspensionForce + lateralForce;
        float dt = GetCore()->DeltaTime();

        // Apply equal and opposite forces to the wheel and the car body.
        mWheelRb->ApplyForce(totalForce * dt, wheelPos);
        mCarRb->ApplyForce(-totalForce * dt, anchorPos);
	}

}