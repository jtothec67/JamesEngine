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

        //BrushTireModel();
		PacejkaTireModel();
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
        //std::cout << "Tire forward: " << tireForward.x << ", " << tireForward.y << ", " << tireForward.z << std::endl;
        //std::cout << "Tire side: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;

        auto ProjectOntoPlane = [&](const glm::vec3& vec, const glm::vec3& n) -> glm::vec3 {
            return vec - n * glm::dot(vec, n);
            };

        glm::vec3 projForward = glm::normalize(ProjectOntoPlane(tireForward, surfaceNormal));
        glm::vec3 projVelocity = ProjectOntoPlane(carVel, surfaceNormal);
        //std::cout << "Projected forward: " << projForward.x << ", " << projForward.y << ", " << projForward.z << std::endl;
        //std::cout << "Projected velocity: " << projVelocity.x << ", " << projVelocity.y << ", " << projVelocity.z << std::endl;

        float Vx = glm::dot(projVelocity, projForward);
        float Vy = glm::dot(projVelocity, tireSide);

        // Use stored angular velocity instead of rigidbody angular velocity
        float wheelCircumferentialSpeed = mWheelAngularVelocity * mTireParams.tireRadius;

        float epsilon = 0.5f;
        float denominator = std::max(std::fabs(wheelCircumferentialSpeed), std::fabs(Vx));
        denominator = std::max(denominator, epsilon);
        float slipRatio = (Vx - wheelCircumferentialSpeed) / denominator;
        float slipAngle = std::atan2(Vy, std::fabs(Vx));
        //std::cout << "Slip ratio: " << slipRatio << std::endl;
        //std::cout << "Slip angle: " << slipAngle << std::endl;

        float Fz = (mCarRb->GetMass() / 4) * 9.81f; // Change to work out load from suspension

        float frictionLimit = mTireParams.peakFrictionCoefficient * Fz;

        float projSpeed = std::abs(Vx);
        float speedScale = glm::clamp(projSpeed / 10.0f, 0.0f, 1.0f);

        //std::cout << "Projected speed: " << projSpeed << std::endl;
        //std::cout << "Speed scale: " << speedScale << std::endl;

        float Fx = -mTireParams.brushLongitudinalStiffness * slipRatio;
        Fx = Fx / (1.0f + std::abs(Fx) / frictionLimit);
        //Fx = 0;
        //std::cout << "Longitudinal force: " << Fx << std::endl;

        float Fy = -mTireParams.brushLateralStiffness * slipAngle * speedScale;
        Fy = Fy / (1.0f + std::abs(Fy) / frictionLimit);
        //Fy = 0;
        //std::cout << "Lateral force: " << Fy << std::endl;

        const float lowSpeedThreshold = 1.f; // m/s
        const float lowOmegaThreshold = 0.5f; // rad/s

        bool carStopped = std::abs(Vx) < lowSpeedThreshold;
        bool wheelStopped = std::abs(mWheelAngularVelocity) < lowOmegaThreshold;

        // To stop the car from sliding when stopped (it still does but only a little now)
        if (carStopped && wheelStopped)
        {
            //Fx = 0.0f;
            //Fy = 0.0f;

            //std::cout << "Car is stopped, no tire forces applied" << std::endl;
        }

        //if (mIsRightTire)
            //Fy = -Fy;

        glm::vec3 forceWorld = projForward * (Fx * 1) + tireSide * (Fy * 1);

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
        //std::cout << "Wheel angular velocity: " << mWheelAngularVelocity << "\n";

        if (GetEntity()->GetTag() == "RRwheel")
        {
            std::cout << "Car velocity magnitude: " << glm::length(carVel) << std::endl;
            //std::cout << "RRwheel: " << std::endl;
            //std::cout << "Tire forward: " << tireForward.x << ", " << tireForward.y << ", " << tireForward.z << std::endl;
            //std::cout << "Tire Side: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;
            //std::cout << "Projected forward: " << projForward.x << ", " << projForward.y << ", " << projForward.z << std::endl;
            //std::cout << "tireSide: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;
            //std::cout << "Vx: " << Vx << std::endl;
            //std::cout << "Vy: " << Vy << std::endl;
            //std::cout << "projVelocity: " << projVelocity.x << ", " << projVelocity.y << ", " << projVelocity.z << std::endl;
            //std::cout << "Slip ratio: " << slipRatio << std::endl;
            //std::cout << "Slip angle: " << slipAngle << std::endl;
            //std::cout << "Fx: " << Fx << std::endl;
            //std::cout << "Fy: " << Fy << std::endl;
            std::cout << "Force world: " << forceWorld.x << ", " << forceWorld.y << ", " << forceWorld.z << std::endl;
            std::cout << "Normalised force: " << glm::normalize(forceWorld).x << ", " << glm::normalize(forceWorld).y << ", " << glm::normalize(forceWorld).z << std::endl;
        }
        if (GetEntity()->GetTag() == "RLwheel")
        {
            std::cout << "RLwheel: " << std::endl;
            //std::cout << "Tire forward: " << tireForward.x << ", " << tireForward.y << ", " << tireForward.z << std::endl;
            //std::cout << "Tire Side: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;
            //std::cout << "Projected forward: " << projForward.x << ", " << projForward.y << ", " << projForward.z << std::endl;
            //std::cout << "tireSide: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;
            //std::cout << "Vx: " << Vx << std::endl;
            //std::cout << "Vy: " << Vy << std::endl;
            //std::cout << "projVelocity: " << projVelocity.x << ", " << projVelocity.y << ", " << projVelocity.z << std::endl;
            //std::cout << "Slip ratio: " << slipRatio << std::endl;
            //std::cout << "Slip angle: " << slipAngle << std::endl;
            //std::cout << "Fx: " << Fx << std::endl;
            //std::cout << "Fy: " << Fy << std::endl;
            std::cout << "Force world: " << forceWorld.x << ", " << forceWorld.y << ", " << forceWorld.z << std::endl;
            std::cout << "Normalised force: " << glm::normalize(forceWorld).x << ", " << glm::normalize(forceWorld).y << ", " << glm::normalize(forceWorld).z << std::endl;
        }
        if (GetEntity()->GetTag() == "FLwheel")
        {
            std::cout << "FLwheel: " << std::endl;
            //std::cout << "Tire forward: " << tireForward.x << ", " << tireForward.y << ", " << tireForward.z << std::endl;
            //std::cout << "Tire Side: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;
            //std::cout << "Projected forward: " << projForward.x << ", " << projForward.y << ", " << projForward.z << std::endl;
            //std::cout << "tireSide: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;
            //std::cout << "Vx: " << Vx << std::endl;
            //std::cout << "Vy: " << Vy << std::endl;
            //std::cout << "projVelocity: " << projVelocity.x << ", " << projVelocity.y << ", " << projVelocity.z << std::endl;
            //std::cout << "Slip ratio: " << slipRatio << std::endl;
            //std::cout << "Slip angle: " << slipAngle << std::endl;
            //std::cout << "Fx: " << Fx << std::endl;
            //std::cout << "Fy: " << Fy << std::endl;
            std::cout << "Force world: " << forceWorld.x << ", " << forceWorld.y << ", " << forceWorld.z << std::endl;
            std::cout << "Normalised force: " << glm::normalize(forceWorld).x << ", " << glm::normalize(forceWorld).y << ", " << glm::normalize(forceWorld).z << std::endl;
        }
        if (GetEntity()->GetTag() == "FRwheel")
        {
            std::cout << "FRwheel: " << std::endl;
            //std::cout << "Tire forward: " << tireForward.x << ", " << tireForward.y << ", " << tireForward.z << std::endl;
            //std::cout << "Tire Side: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;
            //std::cout << "Projected forward: " << projForward.x << ", " << projForward.y << ", " << projForward.z << std::endl;
            //std::cout << "tireSide: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;
            //std::cout << "Vx: " << Vx << std::endl;
            //std::cout << "Vy: " << Vy << std::endl;
            //std::cout << "projVelocity: " << projVelocity.x << ", " << projVelocity.y << ", " << projVelocity.z << std::endl;
            //std::cout << "Slip ratio: " << slipRatio << std::endl;
            //std::cout << "Slip angle: " << slipAngle << std::endl;
            //std::cout << "Fx: " << Fx << std::endl;
            //std::cout << "Fy: " << Fy << std::endl;
            std::cout << "Force world: " << forceWorld.x << ", " << forceWorld.y << ", " << forceWorld.z << std::endl;
            std::cout << "Normalised force: " << glm::normalize(forceWorld).x << ", " << glm::normalize(forceWorld).y << ", " << glm::normalize(forceWorld).z << std::endl;
        }

        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;

        /*std::cout << GetEntity()->GetTag()
            << " SlipAngle: " << glm::degrees(slipAngle)
            << " Vy: " << Vy
            << " Fy: " << Fy
            << " tireSide: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z
            << "\n";*/

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

	void Tire::PacejkaTireModel()
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
        glm::vec3 tireForward = glm::normalize(wheelTransform->GetForward());
        glm::vec3 tireSide = glm::normalize(wheelTransform->GetRight());
		if (mIsRightTire)
		{
			tireSide = -tireSide;
		}

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
        float Vy = glm::dot(projVelocity, tireSide);

        // Wheel spin speed converted to road speed
        float r = mTireParams.tireRadius;
        float wheelCircumferentialSpeed = mWheelAngularVelocity * r;

        // Slip ratio and slip angle calculations
        float epsilon = 0.1f;
        float denom = std::max(std::max(std::fabs(Vx), std::fabs(wheelCircumferentialSpeed)), epsilon);
        float slipRatio = (wheelCircumferentialSpeed - Vx) / denom;
        float slipAngle = std::atan2(Vy, std::fabs(Vx) + 1e-4f);

        // Normal load per tire (static split)
        float Fz = (mCarRb->GetMass() / 4.0f) * 9.81f;

        // Pacejka longitudinal parameters
        float Bx = mTireParams.paceLongStiff;
        float Cx = mTireParams.paceLongShape;
        float Dx = mTireParams.paceLongPeakFriction * Fz;
        float Ex = mTireParams.paceLongCurve;

        // Pacejka longitudinal force
        float argx = Bx * slipRatio;
        float Fx_pacejka = Dx * std::sin(Cx * std::atan(argx - Ex * (argx - std::atan(argx))));

        // Pacejka lateral parameters
        float By = mTireParams.paceLatStiff;
        float Cy = mTireParams.paceLatShape;
        float Dy = mTireParams.paceLatPeakFriction * Fz;
        float Ey = mTireParams.paceLatCurve;

        // Pacejka lateral force
        float argy = By * slipAngle;
        float Fy_pacejka = Dy * std::sin(Cy * std::atan(argy - Ey * (argy - std::atan(argy))));

        // Combined friction limit (friction circle)
        float frictionLimit = mTireParams.peakFrictionCoefficient * Fz;
        glm::vec2 ff = glm::vec2(Fx_pacejka, Fy_pacejka);
        float mag = glm::length(ff);
        if (mag > frictionLimit)
        {
            ff *= (frictionLimit / mag);
        }
        float Fx = ff.x;
        float Fy = ff.y;

        // Apply the tire force to the car body
        glm::vec3 forceWorld = projForward * Fx + projSide * Fy;
        mCarRb->ApplyForce(forceWorld, anchorPos);

        // Compute road reaction torque and net torque on the wheel
        float roadTorque = -Fx * r;
        float netTorque = mDriveTorque + roadTorque;

        // Apply brake torque if it opposes current spin
        if (mBrakeTorque > 0.0f)
        {
            float brakeDir = -glm::sign(mWheelAngularVelocity);
            float resistingTq = brakeDir * mBrakeTorque;
            if (glm::sign(resistingTq) == glm::sign(mWheelAngularVelocity))
            {
                netTorque += resistingTq;
            }
            // Clamp wheel to zero if nearly stopped
            if (std::fabs(mWheelAngularVelocity) < 1.0f)
            {
                mWheelAngularVelocity = 0.0f;
                netTorque = 0.0f;
            }
        }

        // Update wheel angular velocity: I = 0.5 * m * r^2
        float inertia = 0.5f * mTireParams.wheelMass * r * r;
        float angAcc = netTorque / inertia;
        mWheelAngularVelocity += angAcc * dt;

        // Reset per-frame torques
        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;


        /*std::cout << GetEntity()->GetTag() << " tireForward: " << tireForward.x << ", " << tireForward.y << ", " << tireForward.z << std::endl;
        std::cout << GetEntity()->GetTag() << " tireSide: " << tireSide.x << ", " << tireSide.y << ", " << tireSide.z << std::endl;
        std::cout << GetEntity()->GetTag() << " Vx: " << Vx << " Vy: " << Vy << std::endl;
        std::cout << GetEntity()->GetTag() << " Slip Ratio: " << slipRatio << " Slip Angle: " << slipAngle << std::endl;
        std::cout << GetEntity()->GetTag() << " Fx_raw: " << Fx_pacejka << " Fy_raw: " << Fy_pacejka << std::endl;
        std::cout << GetEntity()->GetTag() << " Fx_clipped: " << Fx << " Fy_clipped: " << Fy << std::endl;
        std::cout << GetEntity()->GetTag() << " forceWorld: "
            << forceWorld.x << ", " << forceWorld.y << ", " << forceWorld.z << std::endl;
        std::cout << GetEntity()->GetTag() << " Wheel Angular Velocity: " << mWheelAngularVelocity << std::endl;
        std::cout << GetEntity()->GetTag() << "Car angular velocity (yaw): " << mCarRb->GetAngularVelocity().y << std::endl;*/
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