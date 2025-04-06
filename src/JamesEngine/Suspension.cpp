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

        // Compute relative velocity along the suspension axis
        glm::vec3 wheelVel = mWheelRb->GetVelocity();
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);
        float relativeVel = glm::dot(wheelVel - carVel, suspensionAxis);

        // Calculate the spring and damper contributions
        float springForceMag = mStiffness * compression;
        float damperForceMag = mDamping * relativeVel;
        glm::vec3 suspensionForce = suspensionAxis * (springForceMag - damperForceMag);

        // Project the wheel position onto the suspension axis line (through anchorPos)
        glm::vec3 projectedPos = anchorPos + suspensionAxis * currentLength;

        // Combine the suspension (spring/damper) force with the lateral correction force.
        glm::vec3 totalForce = suspensionForce;
        float dt = GetCore()->DeltaTime();

        // Apply equal and opposite forces to the wheel and the car body.
        mWheelRb->ApplyForce(totalForce * dt, wheelPos);
        mCarRb->ApplyForce(-totalForce * dt, anchorPos);

        // Correct the wheel laterally over time to avoid snapping
        float alpha = 0.5f;
        glm::vec3 correctedPos = glm::mix(wheelTransform->GetPosition(), projectedPos, alpha);
        wheelTransform->SetPosition(correctedPos);
	}

}