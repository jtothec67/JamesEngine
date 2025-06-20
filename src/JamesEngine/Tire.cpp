#include "Tire.h"

#include "Transform.h"
#include "Core.h"
#include "Entity.h"
#include "Rigidbody.h"
#include "Suspension.h"
#include "ModelRenderer.h"
#include "AudioSource.h"
#include "Resources.h"

namespace JamesEngine
{

    void Tire::OnAlive()
    {
        mAudioSource = GetEntity()->AddComponent<AudioSource>();
		mAudioSource->SetSound(GetCore()->GetResources()->Load<Sound>("sounds/tire screech"));
		mAudioSource->SetLooping(true);

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
        float dt = GetCore()->FixedDeltaTime();

        // If wheel is off the ground, don't do tire model, just deal with inputs
        if (!mSuspension->GetCollision())
        {
            mIsSliding = false;

            float netTorque = mDriveTorque;

            if (mBrakeTorque > 0.0f)
            {
                float brakeDirection = -glm::sign(mWheelAngularVelocity);
                float resistingTorque = brakeDirection * mBrakeTorque;

                // Only apply brake torque if it's resisting the current spin
                if (glm::sign(resistingTorque) == -glm::sign(mWheelAngularVelocity))
                {
                    netTorque += resistingTorque;
                }
            }

            // Slow wheel down when in air (simple damping)
            float wheelDampingCoeff = 2.f;
            float dragTorque = -wheelDampingCoeff * mWheelAngularVelocity;
            netTorque += dragTorque;

            // Compute inertia and angular acceleration
            float r = mTireParams.tireRadius;
            float inertia = 0.5f * (mTireParams.wheelMass * 10) * r * r;
            float angularAcceleration = netTorque / inertia;
            mWheelAngularVelocity += angularAcceleration * dt;

            // Reset drive and brake torques, silence audio
            mDriveTorque = 0.0f;
            mBrakeTorque = 0.0f;
            mAudioSource->SetGain(0.0f);

            return;
        }

        // Compute vehicle velocity at contact
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(mSuspension->GetContactPoint());

        // Build contact plane basis vectors
        glm::vec3 tireForward = glm::normalize(GetEntity()->GetComponent<Transform>()->GetForward());
        auto ProjectOntoPlane = [&](const glm::vec3& vec, const glm::vec3& n) -> glm::vec3 {
            return vec - n * glm::dot(vec, n);
            };
        glm::vec3 surfaceNormal = mSuspension->GetSurfaceNormal();
        glm::vec3 projForward = glm::normalize(ProjectOntoPlane(tireForward, surfaceNormal));
        glm::vec3 projSide = glm::normalize(glm::cross(surfaceNormal, projForward));
        glm::vec3 projVelocity = ProjectOntoPlane(carVel, surfaceNormal);

		// Decompose velocity into longitudinal and lateral (Vx long, Vy lat)
        float Vx = glm::dot(projVelocity, projForward);
        float Vy = glm::dot(projVelocity, projSide);

        // Compute slip ratio and angle based on wheel rotation and ground speed
        float wheelCircumferentialSpeed = mWheelAngularVelocity * mTireParams.tireRadius;
        float denominator = std::max(std::fabs(wheelCircumferentialSpeed), std::fabs(Vx));
		if (denominator < 0.001f) // Avoid division by zero
			denominator = 0.001f;
        float slipRatio = (Vx - wheelCircumferentialSpeed) / denominator;

        float VxClamped = std::max(std::fabs(Vx), 0.5f);
        float slipAngle = std::atan2(Vy, VxClamped);

        // Compute vertical load from suspension compression and weight transfer
        float Fz = mSuspension->GetForce();

        // Determine tire stiffness and maximum friction force
        float longStiff = mTireParams.brushLongStiffCoeff * Fz;
        float latStiff = mTireParams.brushLatStiffCoeff * Fz;

        float slipRatioSquared = (longStiff * slipRatio) * (longStiff * slipRatio);
        float slipAngleSquared = (latStiff * glm::tan(slipAngle)) * (latStiff * glm::tan(slipAngle));
        float gamma = glm::sqrt(slipAngleSquared + slipRatioSquared);
        float Fmax = mTireParams.peakFrictionCoefficient * Fz;

        // Compute friction forces and handle grip vs sliding behavior
        float Fx, Fy;
        float maxBrakeTorqueTransferable = Fmax * mTireParams.tireRadius;
        bool tooMuchBrake = (mBrakeTorque > maxBrakeTorqueTransferable);

		//std::cout << GetEntity()->GetTag() << " Fmax: " << Fmax << std::endl;

        if (gamma < Fmax)
        {
            // Elastic (grip) region: proportional longitudinal and lateral forces
            Fx = -longStiff * slipRatio;
            Fy = -latStiff * glm::tan(slipAngle);

            Fx = glm::clamp(Fx, -Fmax, Fmax);
            Fy = glm::clamp(Fy, -Fmax, Fmax);

            mIsSliding = false;
        }
        else
        {
            // Sliding region: friction limited by Fmax
            float forceRatioX = -longStiff * slipRatio;
            float forceRatioY = -latStiff * glm::tan(slipAngle);

            Fx = Fmax * (forceRatioX / gamma);
            Fy = Fmax * (forceRatioY / gamma);

            mIsSliding = true;

			// Check for excessive braking resulting in lockup
			bool excessiveSlip = std::abs(slipRatio) > 0.175f; // Arbitrary threshold
            if (tooMuchBrake && excessiveSlip)
            {
                mIsLocked = true;
            }
        }

  //      if (mDriveTorque == 0)
		//	projForward = glm::vec3(0, 0, 0); // No drive torque means no forward force

  //      // Convert to world space and apply tire and rolling resistance forces
  //      glm::vec3 forceWorld = projForward * Fx + projSide * Fy;
  //      mCarRb->ApplyForce(forceWorld, mSuspension->GetContactPoint());

		//std::cout << GetEntity()->GetTag() << " tire force: " << glm::length(forceWorld) << std::endl;
  //      glm::vec3 rollingResistanceDir = -tireForward * glm::sign(Vx);
  //      glm::vec3 rollingResistanceForce = rollingResistanceDir * mTireParams.rollingResistance * Fz;
  //      mCarRb->ApplyForce(rollingResistanceForce, mSuspension->GetContactPoint());
		////std::cout << GetEntity()->GetTag() << " Rolling resistance: " << glm::length(rollingResistanceForce) << std::endl;

  //      // Update wheel angular velocity with drive, road, and brake torques, handle lock
  //      float roadTorque = -Fx * mTireParams.tireRadius;
  //      float netTorque = mDriveTorque + roadTorque;
  //      if (mBrakeTorque > 0.0f)
  //      {
		//	// Apply brake torque if it's resisting spin
  //          float brakeDirection = -glm::sign(mWheelAngularVelocity);
  //          float resistingTorque = brakeDirection * mBrakeTorque;
  //          if (glm::sign(resistingTorque) == -glm::sign(mWheelAngularVelocity))
  //          {
  //              netTorque += resistingTorque;
  //          }
  //      }

  //      if (mIsLocked)
  //      {
  //          // Wheel locked: no rotation and full sliding friction
  //          mWheelAngularVelocity = 0.0f;
  //          Fx = -mTireParams.peakFrictionCoefficient * Fz * glm::sign(Vx);
  //          Fy = -mTireParams.peakFrictionCoefficient * Fz * glm::tan(slipAngle);
  //          roadTorque = 0.0f;
  //      }

  //      if (mBrakeTorque <= maxBrakeTorqueTransferable || !mIsSliding)
  //      {
  //          mIsLocked = false;
  //      }

  //      // Compute inertia and angular acceleration
  //      float inertia = 0.5f * (mTireParams.wheelMass * 10) * mTireParams.tireRadius * mTireParams.tireRadius;
  //      float angularAcceleration = netTorque / inertia;
  //      mWheelAngularVelocity += angularAcceleration * dt;
  //      mDriveTorque = 0.0f;
  //      mBrakeTorque = 0.0f;


        // Compute max longitudinal force from available torque
        float effectiveBrakeTorque = 0.0f;
        if (mBrakeTorque > 0.0f)
        {
            float brakeDir = -glm::sign(mWheelAngularVelocity);
            float resistingTorque = brakeDir * mBrakeTorque;

            if (glm::sign(resistingTorque) == -glm::sign(mWheelAngularVelocity))
            {
                effectiveBrakeTorque = resistingTorque;
            }
        }

        float torqueAvailable = mDriveTorque + effectiveBrakeTorque;
        float maxFx = std::abs(torqueAvailable / mTireParams.tireRadius);

        // Clamp Fx to respect drivetrain limits
        if (std::abs(mDriveTorque) > 0.01f || std::abs(effectiveBrakeTorque) > 0.01f)
        {
            float torqueAvailable = mDriveTorque + effectiveBrakeTorque;
            float maxFx = std::abs(torqueAvailable / mTireParams.tireRadius);
            Fx = glm::clamp(Fx, -maxFx, maxFx);
        }

        // Apply Fx to car only if drive or brake torque exists
        if (std::abs(mDriveTorque) > 0.01f || std::abs(mBrakeTorque) > 0.01f)
        {
            glm::vec3 forceWorld = projForward * Fx + projSide * Fy;
            mCarRb->ApplyForce(forceWorld, mSuspension->GetContactPoint());
            std::cout << GetEntity()->GetTag() << " tire force: " << glm::length(forceWorld) << std::endl;
        }
        else
        {
            // Only apply lateral force + rolling resistance
            glm::vec3 forceWorld = projSide * Fy;
            mCarRb->ApplyForce(forceWorld, mSuspension->GetContactPoint());
            std::cout << GetEntity()->GetTag() << " tire force: " << glm::length(forceWorld) << std::endl;
        }

        // Step 6: Apply rolling resistance
        glm::vec3 rollingResistanceDir = -tireForward * glm::sign(Vx);
        glm::vec3 rollingResistanceForce = rollingResistanceDir * mTireParams.rollingResistance * Fz;
        mCarRb->ApplyForce(rollingResistanceForce, mSuspension->GetContactPoint());

        //Step 7: Compute and apply wheel angular acceleration
        float roadTorque = -Fx * mTireParams.tireRadius;
        float netTorque = mDriveTorque + effectiveBrakeTorque + roadTorque;

        if (mIsLocked)
        {
            mWheelAngularVelocity = 0.0f;
            Fx = -mTireParams.peakFrictionCoefficient * Fz * glm::sign(Vx);
            Fy = -mTireParams.peakFrictionCoefficient * Fz * glm::tan(slipAngle);
            roadTorque = 0.0f;
            netTorque = 0.0f;
        }

        if (mBrakeTorque <= maxBrakeTorqueTransferable || !mIsSliding)
        {
            mIsLocked = false;
        }

        float inertia = 0.5f * (mTireParams.wheelMass * 10.0f) * mTireParams.tireRadius * mTireParams.tireRadius;
        float angularAcceleration = netTorque / inertia;
        mWheelAngularVelocity += angularAcceleration * dt;

        // ----- Step 8: Clear torques for next frame -----
        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;

        // Set tire screech audio based on slip conditions
        float tireScreechVolume = 0.0f;
        if (mIsSliding && glm::length(carVel) > 5)
        {
            if (slipRatio > 0.0f)
                tireScreechVolume = glm::clamp(slipRatio, 0.f, 1.f);
            else
                tireScreechVolume = glm::clamp(-slipRatio * 4, 0.f, 1.f);
            mAudioSource->SetGain(tireScreechVolume);
        }
        else
        {
            mAudioSource->SetGain(0.0f);
        }
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

    float Tire::GetSlidingAmount()
    {
        if (mIsSliding)
        {
            return glm::length(mCarRb->GetVelocityAtPoint(mSuspension->GetContactPoint()) - mCarRb->GetVelocity());
        }
        return 0.f;
    }

}