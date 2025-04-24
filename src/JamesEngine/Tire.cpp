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
		if (!mSuspension->GetCollision())
			return;

		//std::cout << GetEntity()->GetTag() << " tire fixed tick" << std::endl;

        BrushTireModel();
		//PacejkaTireModel();
        //Testing();
	}

	void Tire::BrushTireModel()
	{
        float dt = GetCore()->FixedDeltaTime();

        auto wheelTransform = GetEntity()->GetComponent<Transform>();
        auto anchorTransform = mAnchorPoint->GetComponent<Transform>();

        glm::vec3 anchorPos = anchorTransform->GetPosition();

        // Surface normal assumed upward
        glm::vec3 surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f); // Change to get from the RayCollider

        // Use velocity of car at anchor point instead of wheel rigidbody
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);

        glm::vec3 tireForward = glm::normalize(wheelTransform->GetForward());
        glm::vec3 tireSide = glm::normalize(-wheelTransform->GetRight());

        auto ProjectOntoPlane = [&](const glm::vec3& vec, const glm::vec3& n) -> glm::vec3 {
            return vec - n * glm::dot(vec, n);
            };

        glm::vec3 projForward = glm::normalize(ProjectOntoPlane(tireForward, surfaceNormal));
        glm::vec3 projSide = glm::normalize(glm::cross(surfaceNormal, projForward));
        glm::vec3 projVelocity = ProjectOntoPlane(carVel, surfaceNormal);

        float Vx = glm::dot(projVelocity, projForward);
        float Vy = glm::dot(projVelocity, projSide);

        // Use stored angular velocity instead of rigidbody angular velocity
        float wheelCircumferentialSpeed = mWheelAngularVelocity * mTireParams.tireRadius;

        float epsilon = 0.5f;
        float denominator = std::max(std::fabs(wheelCircumferentialSpeed), std::fabs(Vx));
        denominator = std::max(denominator, epsilon);
        float slipRatio = (Vx - wheelCircumferentialSpeed) / denominator;
        float slipAngle = std::atan2(Vy, std::fabs(Vx));

        // Compute gamma (combined slip deformation in tire's contact patch)
        float slipRatioSquared = (mTireParams.brushLongStiff * slipRatio) * (mTireParams.brushLongStiff * slipRatio);
        float slipAngleSquared = (mTireParams.brushLatStiff * glm::tan(slipAngle)) * (mTireParams.brushLatStiff * glm::tan(slipAngle));
        float gamma = glm::sqrt(slipAngleSquared + slipRatioSquared);

        // Vertical load
        float Fz = (mCarRb->GetMass() / 4) * 9.81f; // Change to work out load from suspension

        // Maximum friction force
        float Fmax = mTireParams.peakFrictionCoefficient * Fz;

        float Fx, Fy;

        if (gamma < Fmax)
        {
            // Elastic region (we have grip)
            Fx = -mTireParams.brushLongStiff * slipRatio;
            Fy = -mTireParams.brushLatStiff * glm::tan(slipAngle);
        }
        else
        {
            // Sliding region
            float forceRatioX = -mTireParams.brushLongStiff * slipRatio;
            float forceRatioY = -mTireParams.brushLatStiff * glm::tan(slipAngle);

            Fx = Fmax * (forceRatioX / gamma);
            Fy = Fmax * (forceRatioY / gamma);

			std::cout << GetEntity()->GetTag() << " is sliding" << std::endl;
        }

		if (std::fabs(Vx) < 1.f)
		{
			//Fy = 0.0f; // No longitudinal force when stationary
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
        float inertia = 0.5f * mTireParams.wheelMass * r * r;
        float angularAcceleration = netTorque / inertia;
        mWheelAngularVelocity += angularAcceleration * dt;

        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;
	}

    void Tire::Testing()
    {
        float dt = GetCore()->FixedDeltaTime();

        // Transforms and anchor point
        auto wheelTransform = GetEntity()->GetComponent<Transform>();
        auto anchorTransform = mAnchorPoint->GetComponent<Transform>();
        glm::vec3 anchorPos = anchorTransform->GetPosition();

        // Ground normal (replace with sampled normal from your ray collider if available)
        glm::vec3 surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);

        // Velocity of the car at the tire contact point
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);

        // Tire local axes
        glm::vec3 tireForward = wheelTransform->GetForward();
        glm::vec3 tireSide = -wheelTransform->GetRight();

        // Project a vector onto the ground plane
        auto ProjectOntoPlane = [&](const glm::vec3& v, const glm::vec3& n) {
            return v - n * glm::dot(v, n);
            };

        // Flatten forward axis and velocity onto ground plane
        glm::vec3 projForward = glm::normalize(ProjectOntoPlane(tireForward, surfaceNormal));
        glm::vec3 projVelocity = ProjectOntoPlane(carVel, surfaceNormal);

        glm::vec3 projSide = glm::normalize(glm::cross(surfaceNormal, projForward));

        // Compute longitudinal (Vx) and lateral (Vy) speeds
        float Vx = glm::dot(projVelocity, projForward);
        float Vy = glm::dot(projVelocity, projSide);

        // Slip angle in radians
        float slipAngle = std::atan2(Vy, std::fabs(Vx));

		// Slip ratio (-1 to 1)
        float wheelCircumferentialSpeed = mWheelAngularVelocity * mTireParams.tireRadius;
        float denominator = (std::fabs(Vx) > 0.1f) ? std::fabs(Vx) : 0.1f;
        float slipRatio = (wheelCircumferentialSpeed - Vx) / denominator;

		// Compute gamma (combined slip deformation in tire's contact patch)
        float slipRatioSquared = (mTireParams.brushLongStiff * slipRatio) * (mTireParams.brushLongStiff * slipRatio);
		float slipAngleSquared = (mTireParams.brushLatStiff * glm::tan(slipAngle)) * (mTireParams.brushLatStiff * glm::tan(slipAngle));
		float gamma = glm::sqrt(slipAngleSquared + slipRatioSquared);

        // Vertical load
        float Fz = (mCarRb->GetMass() / 4) * 9.81f; // Change to work out load from suspension

		// Maximum friction force
		float Fmax = mTireParams.peakFrictionCoefficient * Fz;

		float Fx, Fy;

        if (gamma < Fmax)
        {
			// Elastic region (we have grip)
			Fx = -mTireParams.brushLongStiff * slipRatio;
			Fy = -mTireParams.brushLatStiff * glm::tan(slipAngle);
        }
        else
        {
            // Sliding region
            float forceRatioX = -mTireParams.brushLongStiff * slipRatio;
			float forceRatioY = -mTireParams.brushLatStiff * glm::tan(slipAngle);

			Fx = Fmax * (forceRatioX / gamma);
			Fy = Fmax * (forceRatioY / gamma);
        }

        glm::vec3 forceWorld = projForward * Fx + projSide * Fy;
		mCarRb->ApplyForce(forceWorld, anchorPos);

		std::cout << GetEntity()->GetTag() << " tireForward: " << tireForward.x << ", " << tireForward.y << ", " << tireForward.z << std::endl;
		std::cout << GetEntity()->GetTag() << " Vx: " << Vx << ", Vy: " << Vy << std::endl;
		std::cout << GetEntity()->GetTag() << " Slip Angle: " << slipAngle << std::endl;
		std::cout << GetEntity()->GetTag() << " Slip Ratio: " << slipRatio << std::endl;
		std::cout << GetEntity()->GetTag() << " gamma: " << gamma << std::endl;
		std::cout << GetEntity()->GetTag() << " Fz: " << Fz << std::endl;
        std::cout << GetEntity()->GetTag() << " forceWorld: " << forceWorld.x << ", " << forceWorld.y << ", " << forceWorld.z << std::endl;
		std::cout << GetEntity()->GetTag() << " forec world length: " << glm::length(forceWorld) << std::endl;
    }

	void Tire::OnTick()
	{
        mWheelRotation += glm::degrees(mWheelAngularVelocity * GetCore()->DeltaTime());

        if (mWheelRotation > 360) {
            mWheelRotation -= 360;
        }
        else if (mWheelRotation < 0.0f) {
            mWheelRotation += 360;
        }

        glm::vec3 modelRotationOffset = glm::vec3(mWheelRotation, 0.0f, 0.0f) + mInitialRotationOffset;
        GetEntity()->GetComponent<ModelRenderer>()->SetRotationOffset(modelRotationOffset);
	}

}