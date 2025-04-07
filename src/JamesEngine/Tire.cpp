#include "Tire.h"

#include "Transform.h"
#include "Core.h"
#include "Entity.h"
#include "Rigidbody.h"

namespace JamesEngine
{

	void Tire::OnTick()
	{
		if (!mCarBody || !mAnchorPoint)
		{
			std::cout << "Tire component is missing a car body, or anchor point" << std::endl;
			return;
		}

		if (!mWheelRb || !mCarRb)
		{
			mWheelRb = GetEntity()->GetComponent<Rigidbody>();
			mCarRb = mCarBody->GetComponent<Rigidbody>();

			if (!mWheelRb || !mCarRb)
			{
				std::cout << "Tire component is missing a rigidbody on the car body" << std::endl;
				return;
			}
		}

        // Get delta time.
        float dt = GetCore()->DeltaTime();

        // Get transforms.
        auto wheelTransform = GetEntity()->GetComponent<Transform>();
        auto anchorTransform = mAnchorPoint->GetComponent<Transform>();

        glm::vec3 wheelPos = wheelTransform->GetPosition();
        glm::vec3 anchorPos = anchorTransform->GetPosition();

        // ------------ CHANGE TO ACTUALLY BE THE SURFACE NORMAL ------------
        glm::vec3 surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);

        // Get velocities.
        glm::vec3 wheelVel = mWheelRb->GetVelocity();
        glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);

        // Get the tire's "rolling direction" (i.e. the direction it is pointed for rolling).
        // This should be set by the steering input. We assume here that GetForward() returns that.
        glm::vec3 tireForward = wheelTransform->GetForward();

        // Compute the tire's lateral (side) direction as the cross product of surface normal and forward.
        glm::vec3 tireSide = glm::normalize(glm::cross(surfaceNormal, tireForward));

        // Helper: project any vector onto the ground plane.
        auto ProjectOntoPlane = [&](const glm::vec3& vec, const glm::vec3& n) -> glm::vec3 {
            return vec - n * glm::dot(vec, n);
            };

        // Project the forward vector and wheel velocity onto the ground.
        glm::vec3 projForward = glm::normalize(ProjectOntoPlane(tireForward, surfaceNormal));
        glm::vec3 projVelocity = ProjectOntoPlane(wheelVel, surfaceNormal);

        // Calculate the longitudinal component of the velocity.
        float Vx = glm::dot(projVelocity, projForward);

        // Obtain the wheel's angular velocity along its axle.
        // Here we assume that the wheel rotates about its local right axis.
        glm::vec3 wheelAxle = wheelTransform->GetRight(); // Adjust if your axle is different.
        glm::vec3 wheelAngular = mWheelRb->GetAngularVelocity();
        float omega = glm::dot(wheelAngular, wheelAxle);
        float wheelCircumferentialSpeed = omega * mTireParams.tireRadius;

        // Compute slip ratio: (wheel speed - longitudinal velocity) / |longitudinal velocity|
        float denominator = (std::fabs(Vx) > 0.1f) ? std::fabs(Vx) : 0.1f;
        float slipRatio = (wheelCircumferentialSpeed - Vx) / denominator;

        // Compute lateral slip angle.
        // Lateral velocity component: projection of projVelocity onto tireSide.
        float Vy = glm::dot(projVelocity, tireSide);
        float slipAngle = std::atan2(Vy, std::fabs(Vx)); // in radians

        // Estimate vertical load (Fz) on the tire.
        // For now we approximate using the wheel's mass and gravity.
        float mass = mWheelRb->GetMass();
        float Fz = mass * 9.81f;

        // --- Brush Tire Model Calculation ---
        // Simple linear model for demonstration:
        float Fx = -mTireParams.longitudinalStiffness * slipRatio;
        float Fy = -mTireParams.lateralStiffness * slipAngle;

        // Clamp forces to a friction limit based on peak friction coefficient.
        float frictionLimit = mTireParams.peakFrictionCoefficient * Fz;
        Fx = glm::clamp(Fx, -frictionLimit, frictionLimit);
        Fy = glm::clamp(Fy, -frictionLimit, frictionLimit);

        // Convert tire-local forces to world space.
        glm::vec3 forceWorld = projForward * Fx + tireSide * Fy;

        // --- Apply Forces ---
        // Apply the tire force to the car body at the anchor point.
        mCarRb->ApplyForce(forceWorld * dt, anchorPos);
        // Apply the opposite force to the wheel (action-reaction).
        mWheelRb->ApplyForce(-forceWorld * dt, wheelPos);

        // --- Update Wheel Angular Velocity ---
        // The force at the tire-road interface produces a torque on the wheel.
        float roadTorque = Fx * mTireParams.tireRadius;
        float netTorque = mDriveTorque - mBrakeTorque - roadTorque;

        // Get the wheel's moment of inertia.
        float r = mTireParams.tireRadius;
        float inertia = 0.5f * mass * r * r;
        float angularAcceleration = netTorque / inertia;
        float newOmega = omega + angularAcceleration * dt;

        // Update the wheel's angular velocity.
        glm::vec3 newAngularVelocity = wheelAxle * newOmega;
        mWheelRb->SetAngularVelocity(newAngularVelocity);

        // Optionally: update any visual wheel spin here by rotating a child transform.


        // Reset drive and brake torques for the next tick.
        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;
	}

}