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

		glm::vec3 anchorPos = mAnchorPoint->GetComponent<Transform>()->GetPosition();
		glm::vec3 wheelPos = mWheel->GetComponent<Transform>()->GetPosition();
		glm::vec3 suspensionDir = mAnchorPoint->GetComponent<Transform>()->GetUp();

		glm::vec3 offset = wheelPos - anchorPos;
		float currentLength = glm::dot(offset, suspensionDir);
		float clampedLength = glm::clamp(currentLength, mMinLength, mMaxLength);
		float compression = mRestLength - clampedLength;

		glm::vec3 wheelVel = mWheelRb->GetVelocity();
		glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);
		float relativeVel = glm::dot(wheelVel - carVel, suspensionDir);

		float springForceMag = mStiffness * compression;
		float damperForceMag = mDamping * relativeVel;
		float totalForceMag = springForceMag - damperForceMag;
		glm::vec3 totalForce = suspensionDir * totalForceMag;

		mWheelRb->ApplyForce(totalForce * GetCore()->DeltaTime(), wheelPos);
		mCarRb->ApplyForce(-totalForce * GetCore()->DeltaTime(), anchorPos);
	}

}