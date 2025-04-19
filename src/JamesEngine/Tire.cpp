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

        float dt = GetCore()->FixedDeltaTime();

        auto wheelTransform = GetEntity()->GetComponent<Transform>();
        auto anchorTransform = mAnchorPoint->GetComponent<Transform>();

        glm::vec3 anchorPos = anchorTransform->GetPosition();

        // Surface normal assumed upward
        glm::vec3 surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);

        // Use velocity of car at anchor point instead of wheel rigidbody
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);
		//std::cout << GetEntity()->GetTag() << " car velocity : " << carVel.x << ", " << carVel.y << ", " << carVel.z << std::endl;

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
        float slipRatio = (wheelCircumferentialSpeed - Vx) / denominator;
        float slipAngle = std::atan2(Vy, std::fabs(Vx));

        // Estimate load (Fz) from suspension or set manually
        float Fz = mTireParams.wheelMass * 9.81f;

        float Fx = -mTireParams.longitudinalStiffness * slipRatio;
        float Fy = -mTireParams.lateralStiffness * slipAngle;

        float frictionLimit = mTireParams.peakFrictionCoefficient * Fz;
        Fx = glm::clamp(Fx, -frictionLimit, frictionLimit);
        Fy = glm::clamp(Fy, -frictionLimit, frictionLimit);

        glm::vec3 forceWorld = projForward * Fx + tireSide * Fy;

        // Apply force to car at the anchor point
        mCarRb->ApplyForce(forceWorld / 10.f, anchorPos);
		//std::cout << GetEntity()->GetTag() << " tire force: " << forceWorld.x << ", " << forceWorld.y << ", " << forceWorld.z << std::endl;

        // Update Simulated Wheel Angular Velocity
        float roadTorque = -Fx * mTireParams.tireRadius;
        float netTorque = mBrakeTorque - mDriveTorque - roadTorque;

        float r = mTireParams.tireRadius;
        float inertia = 0.5f * mTireParams.wheelMass * r * r;
        float angularAcceleration = netTorque / inertia;
        mWheelAngularVelocity += angularAcceleration * dt;

		//std::cout << GetEntity()->GetTag() << " wheel angular velocity: " << mWheelAngularVelocity << std::endl;

        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;
	}

	void Tire::OnTick()
	{
        mWheelRotation += mWheelAngularVelocity * GetCore()->DeltaTime();

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