#include "Tire.h"

#include "Transform.h"
#include "Core.h"
#include "Entity.h"
#include "Rigidbody.h"
#include "Suspension.h"
#include "ModelRenderer.h"

namespace JamesEngine
{

    void Tire::OnAlive()
    {
		if (!mCarBody || !mAnchorPoint)
		{
			std::cout << "Tire component is missing a car body or anchor point" << std::endl;
			return;
		}

		mCarRb = mCarBody->GetComponent<Rigidbody>();
		if (!mCarRb)
		{
			std::cout << "Tire component is missing a rigidbody on the car body" << std::endl;
			return;
		}

        mSuspension = GetEntity()->GetComponent<Suspension>();
		if (!mSuspension)
		{
			std::cout << "Tire is missing a suspension component" << std::endl;
			return;
		}
    }

	void Tire::OnFixedTick()
	{
        BrushTireModel();
	}

	void Tire::BrushTireModel()
	{
        if (!mSuspension->GetCollision())
            return;

        float dt = GetCore()->FixedDeltaTime();

        auto wheelTransform = GetEntity()->GetComponent<Transform>();
        auto anchorTransform = mAnchorPoint->GetComponent<Transform>();

        glm::vec3 anchorPos = anchorTransform->GetPosition();

        // Use velocity of car at anchor point instead of wheel rigidbody
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);

        glm::vec3 tireForward = glm::normalize(wheelTransform->GetForward());

        auto ProjectOntoPlane = [&](const glm::vec3& vec, const glm::vec3& n) -> glm::vec3 {
            return vec - n * glm::dot(vec, n);
            };

        glm::vec3 surfaceNormal = mSuspension->GetSurfaceNormal();

        glm::vec3 projForward = glm::normalize(ProjectOntoPlane(tireForward, surfaceNormal));
        glm::vec3 projSide = glm::normalize(glm::cross(surfaceNormal, projForward));
        glm::vec3 projVelocity = ProjectOntoPlane(carVel, surfaceNormal);

        float Vx = glm::dot(projVelocity, projForward);
        float Vy = glm::dot(projVelocity, projSide);

        // Use stored angular velocity instead of rigidbody angular velocity
        float wheelCircumferentialSpeed = mWheelAngularVelocity * mTireParams.tireRadius;

        float epsilon = 0.01f;
        float denominator = std::max(std::fabs(wheelCircumferentialSpeed), std::fabs(Vx));
        float slipRatio = (Vx - wheelCircumferentialSpeed) / denominator;
        float slipAngle = std::atan2(Vy, std::fabs(Vx));

        // Vertical load
		float suspensionCompression = mSuspension->GetCompression();
        float baseWeight = mCarRb->GetMass() / 4.0f * 9.81f;

        // Weight transfer coefficient (how much load shifts with suspension movement)
        float weightTransferCoeff = 20000.0f;
        float additionalLoad = suspensionCompression * weightTransferCoeff;

        float Fz = baseWeight + additionalLoad;
		//std::cout << GetEntity()->GetTag() << " Fz: " << Fz << " compression: " << suspensionCompression << std::endl;

        float longStiff = mTireParams.brushLongStiffCoeff * Fz;
        float latStiff = mTireParams.brushLatStiffCoeff * Fz;

        // Compute gamma (combined slip deformation in tire's contact patch)
        float slipRatioSquared = (longStiff * slipRatio) * (longStiff * slipRatio);
        float slipAngleSquared = (latStiff * glm::tan(slipAngle)) * (latStiff * glm::tan(slipAngle));
        float gamma = glm::sqrt(slipAngleSquared + slipRatioSquared);

        // Maximum friction force
        float Fmax = mTireParams.peakFrictionCoefficient * Fz;
        //std::cout << GetEntity()->GetTag() << " Fmax: " << Fmax << std::endl;

        float Fx, Fy;

        if (gamma < Fmax)
        {
            // Elastic region (we have grip)
            Fx = -longStiff * slipRatio;
            Fy = -latStiff * glm::tan(slipAngle);
        }
        else
        {
            // Sliding region
            float forceRatioX = -longStiff * slipRatio;
            float forceRatioY = -latStiff * glm::tan(slipAngle);

            Fx = Fmax * (forceRatioX / gamma);
            Fy = Fmax * (forceRatioY / gamma);

			//std::cout << GetEntity()->GetTag() << "Tire is sliding! Fx: " << Fx << ", Fy: " << Fy << std::endl;
			//std::cout << GetEntity()->GetTag() << " slip Ratio: " << slipRatio << ", Slip Angle: " << slipAngle << std::endl;
	    }

        glm::vec3 forceWorld = projForward * Fx + projSide * Fy;

        // Apply force to car at the anchor point
        mCarRb->ApplyForce(forceWorld, anchorPos);

        // Update Simulated Wheel Angular Velocity
        float roadTorque = -Fx * mTireParams.tireRadius;
        float netTorque = mDriveTorque + roadTorque;

        // Add brake torque only if it's resisting current spin
        if (mBrakeTorque > 0.0f)
        {
            float brakeDirection = -glm::sign(mWheelAngularVelocity); // Opposes spin
            float resistingTorque = brakeDirection * mBrakeTorque;

            // Only apply brake torque if it's resisting the current spin
            if (glm::sign(resistingTorque) == -glm::sign(mWheelAngularVelocity))
            {
                netTorque += resistingTorque;
            }

            // If the wheel is nearly stopped and brake is active, hold it at 0
            const float angularStopThreshold = 1.0f; // rad/s
            if (std::abs(mWheelAngularVelocity) < angularStopThreshold)
            {
                mWheelAngularVelocity = 0.0f;
                netTorque = 0.0f; // Don't apply any torque when clamped
            }
        }

        float r = mTireParams.tireRadius;
        float inertia = 0.5f * (mTireParams.wheelMass * 10) * r * r;
        float angularAcceleration = netTorque / inertia;
        mWheelAngularVelocity += angularAcceleration * dt;

        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;

        //std::cout << GetEntity()->GetTag() << " wheel speed: " << mWheelAngularVelocity << std::endl;
        //std::cout << GetEntity()->GetTag() << " slip ratio: " << slipRatio << std::endl;
        //std::cout << GetEntity()->GetTag() << " slip ratio: " << slipRatio << " Fx: " << Fx << ", Fy: " << Fy << std::endl;
	}

	void Tire::OnTick()
	{
        mWheelRotation += glm::degrees(mWheelAngularVelocity * GetCore()->DeltaTime());

        if (mWheelRotation > 360)
        {
            mWheelRotation -= 360;
        }
        else if (mWheelRotation < 0.0f)
        {
            mWheelRotation += 360;
        }

        glm::vec3 modelRotationOffset = glm::vec3(mWheelRotation, 0.0f, 0.0f) + mInitialRotationOffset;
        GetEntity()->GetComponent<ModelRenderer>()->SetRotationOffset(modelRotationOffset);
	}

}