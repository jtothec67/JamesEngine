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

		std::cout << GetEntity()->GetTag() << " tire fixed tick" << std::endl;

        float dt = GetCore()->FixedDeltaTime();

        auto wheelTransform = GetEntity()->GetComponent<Transform>();
        auto anchorTransform = mAnchorPoint->GetComponent<Transform>();

        glm::vec3 anchorPos = anchorTransform->GetPosition();

        // Surface normal assumed upward
        glm::vec3 surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f); // Change to get from the RayCollider

        // Use velocity of car at anchor point instead of wheel rigidbody
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);

        glm::vec3 tireForward = wheelTransform->GetForward();
        glm::vec3 tireSide = glm::normalize(glm::cross(surfaceNormal, tireForward));

        auto ProjectOntoPlane = [&](const glm::vec3& vec, const glm::vec3& n) -> glm::vec3 {
            return vec - n * glm::dot(vec, n);
            };

        glm::vec3 projForward = glm::normalize(ProjectOntoPlane(tireForward, surfaceNormal));
        glm::vec3 projVelocity = ProjectOntoPlane(carVel, surfaceNormal);

        float Vx = glm::dot(projVelocity, projForward);
        float Vy = glm::dot(projVelocity, tireSide);

        // Use stored angular velocity instead of rigidbody angular velocity
        float wheelCircumferentialSpeed = mWheelAngularVelocity * mTireParams.tireRadius;

        float epsilon = 0.5f;
        float denominator = std::max(std::fabs(Vx), epsilon);
        float slipRatio = (Vx - wheelCircumferentialSpeed) / denominator;
        float slipAngle = std::atan2(Vy, std::fabs(Vx));
		std::cout << "Slip ratio: " << slipRatio << std::endl;
		std::cout << "Slip angle: " << slipAngle << std::endl;

        float Fz = (mTireParams.wheelMass + (1230 / 4)) * 9.81f; // Change to work out load from suspension

        float Fx = -mTireParams.longitudinalStiffness * slipRatio;
        Fx = Fx / (1.0f + std::abs(Fx) / (mTireParams.peakFrictionCoefficient * Fz));
		std::cout << "Longitudinal force: " << Fx << std::endl;
        
        float Fy = -mTireParams.lateralStiffness * slipAngle;
        Fy = Fy / (1.0f + std::abs(Fy) / (mTireParams.peakFrictionCoefficient * Fz));
		std::cout << "Lateral force: " << Fy << std::endl;

        float frictionLimit = mTireParams.peakFrictionCoefficient * Fz;

        Fx = glm::clamp(Fx, -frictionLimit, frictionLimit);
        Fy = glm::clamp(Fy, -frictionLimit, frictionLimit);
        
        const float lowSpeedThreshold = 0.5f; // m/s
        const float lowOmegaThreshold = 1.0f; // rad/s

        bool carStopped = std::abs(Vx) < lowSpeedThreshold;
        bool wheelStopped = std::abs(mWheelAngularVelocity) < lowOmegaThreshold;

		// To stop the car from sliding when stopped (it still does but only a little now)
        if (carStopped && wheelStopped)
        {
            slipRatio = 0.0f;
            Fx = 0.0f;
			Fy = 0.0f;

            std::cout << "Car is stopped, no tire forces applied" << std::endl;
        }

        glm::vec3 forceWorld = projForward * Fx + tireSide * Fy;

        // Apply force to car at the anchor point
        mCarRb->ApplyForce(forceWorld / 10.f, anchorPos);

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
		//std::cout << "Wheel angular velocity: " << mWheelAngularVelocity << "\n";

        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;

  //      float stiffness = 100000.0f;
  //      Fz = 3000.0f;
  //      float mu = 1.0f;
  //      frictionLimit = mu * Fz;

  //      // Longitudinal
		//std::cout << "Longitudinal slip vs force:\n";
  //      for (float slip = -1.0f; slip <= 1.0f; slip += 0.01f)
  //      {
  //          float Fx_linear = -stiffness * slip;
  //          float Fx_clamped = glm::clamp(Fx_linear, -frictionLimit, frictionLimit);
  //          float Fx_soft = Fx_linear / (1.0f + std::abs(Fx_linear) / frictionLimit);

  //          std::cout << slip << "," << Fx_linear << "," << Fx_clamped << "," << Fx_soft << "\n";
  //      }

  //      // Lateral
		//std::cout << "Lateral slip vs force:\n";
  //      for (float angle = -0.5f; angle <= 0.5f; angle += 0.01f)
  //      {
  //          float Fy_linear = -stiffness * angle;
  //          float Fy_clamped = glm::clamp(Fy_linear, -frictionLimit, frictionLimit);
  //          float Fy_soft = Fy_linear / (1.0f + std::abs(Fy_linear) / frictionLimit);

  //          std::cout << angle << "," << Fy_linear << "," << Fy_clamped << "," << Fy_soft << "\n";
  //      }
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