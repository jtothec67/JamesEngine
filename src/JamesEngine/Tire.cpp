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
        float Fmax = mTireParams.peakFrictionCoefficient * Fz;

		float FxRaw = -longStiff * slipRatio;
		float FyRaw = -latStiff * glm::tan(slipAngle);

        // Compute gamma (total requested force magnitude)
        float gamma = glm::sqrt(FxRaw * FxRaw + FyRaw * FyRaw);

        // Compute friction forces and handle grip vs sliding behavior
        float Fx, Fy;

        if (gamma < Fmax)
        {
            // Elastic (grip) region: proportional longitudinal and lateral forces
            Fx = FxRaw;
            Fy = FyRaw;

            mIsSliding = false;
        }
        else
        {
            // Sliding region: friction limited by Fmax
            // Outside the ellipse — scale forces to stay within Fmax
            float scale = Fmax / gamma;

            Fx = FxRaw * scale;
            Fy = FyRaw * scale;

            mIsSliding = true;
        }

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
        }
        else
        {
            // Only apply lateral force + rolling resistance
            glm::vec3 forceWorld = projSide * Fy;
            mCarRb->ApplyForce(forceWorld, mSuspension->GetContactPoint());
        }

        // Apply rolling resistance
        glm::vec3 rollingResistanceDir = -tireForward * glm::sign(Vx);
        glm::vec3 rollingResistanceForce = rollingResistanceDir * mTireParams.rollingResistance * Fz;
        mCarRb->ApplyForce(rollingResistanceForce, mSuspension->GetContactPoint());

        // Compute and apply wheel angular acceleration
        float roadTorque = -Fx * mTireParams.tireRadius;
        float netTorque = mDriveTorque + effectiveBrakeTorque + roadTorque;

        float inertia = 0.5f * (mTireParams.wheelMass * 10.0f) * mTireParams.tireRadius * mTireParams.tireRadius;
        float angularAcceleration = netTorque / inertia;
        mWheelAngularVelocity += angularAcceleration * dt;

        // Clear torques for next frame
        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;

        mGripUsage = glm::sqrt(FxRaw * FxRaw + FyRaw * FyRaw) / Fmax;

		std::cout << GetEntity()->GetTag() << " Grip Usage: " << mGripUsage << std::endl;

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