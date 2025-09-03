#include "Tire.h"

#include "Suspension.h"

#include <iomanip>

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

        {
            /*float Fz = 5500.0f;
            float mu = mTireParams.peakFrictionCoefficient;
            float a = mTireParams.contactHalfLengthX;
            float b = mTireParams.contactHalfLengthY;

            float longStiff = mTireParams.brushLongStiffCoeff * Fz;
            float latStiff = mTireParams.brushLatStiffCoeff * Fz;

            float kxA = longStiff / (2.0f * a * b);
            float kyA = latStiff / (2.0f * a * b);
            float p = Fz / (4.0f * a * b);

            float slipRatio = 0.0f;

            const float degStart = 0.0f;
            const float degEnd = 15.0f;
            const float degStep = 0.1f;

            std::cout << "Slip_angle_rad,Fy_force\n";

            for (float aDeg = degStart; aDeg <= degEnd + 1e-6f; aDeg += degStep)
            {
                float slipAngle = aDeg * float(M_PI) / 180.0f;
                float tanAlpha = std::tan(slipAngle);

                float tx = kxA * slipRatio;
                float ty = kyA * tanAlpha;
                float S = std::sqrt(tx * tx + ty * ty);

                float Fy = 0.0f;
                const float Fmax = mu * Fz;

                if (S > 1e-12f && Fz > 0.0f)
                {
                    float xs = 2.0f * a * (mu * p) / S - a;
                    xs = glm::clamp(xs, -a, a);

                    float c = tx / S;
                    float s = ty / S;

                    float factor = (xs + a);
                    float Fy_adh = (2.0f * b) * (kyA * tanAlpha) * (factor * factor / (4.0f * a));

                    float slidingFraction = (a - xs) / (2.0f * a);
                    float slideFriction = mTireParams.slidingFrictionFactor * mu;
                    float effectiveFriction = slideFriction
                        + (mu - slideFriction) * std::pow(1.0f - slidingFraction, mTireParams.slidingFrictionFalloffExponent);

                    float Fy_sl = (2.0f * b) * (effectiveFriction * p * s * (a - xs));

                    Fy = -(Fy_adh + Fy_sl);
                }

                std::cout << std::fixed << std::setprecision(4) << slipAngle << "," << -Fy << "\n";
            }*/
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

        float slipRatioDenom = glm::max(std::fabs(Vx), 0.5f);
        float slipRatio = (wheelCircumferentialSpeed - Vx) / slipRatioDenom;
        slipRatio = glm::clamp(slipRatio, -3.0f, 3.0f);

        float slipAngleDenom = glm::max(std::fabs(Vx), 1.0f);
        float tanSlipAngle = Vy / slipAngleDenom;

        // Compute vertical load from suspension compression and weight transfer
        float Fz = mSuspension->GetForce();

        // Determine tire stiffness and maximum friction force
        float FzRef = mTireParams.loadSensitivityRef;
        float loadScale = glm::max(Fz, 1.0f) / glm::max(FzRef, 1.0f);

        // Stiffness with load sensitivity
        float Cx = mTireParams.longStiffCoeff * FzRef * std::pow(loadScale, mTireParams.longStiffExp);
        float Cy = mTireParams.latStiffCoeff * FzRef * std::pow(loadScale, mTireParams.latStiffExp);

        // Geometric half-dimensions of the footprint
        float a = mTireParams.contactHalfLengthX;
        float b = mTireParams.contactHalfLengthY;

        // Convert to per-area bristle stiffness (preserves small-slip slopes)
        float kxA = Cx / (2.0f * a * b);
        float kyA = Cy / (2.0f * a * b);

        // Uniform pressure over rectangular patch
        float p = Fz / (4.0f * a * b);

        // Build combined shear slope (per-area)
        float tx = kxA * slipRatio;
        float ty = kyA * tanSlipAngle;
        float S = glm::sqrt(tx * tx + ty * ty);

        // Compute friction forces and handle adhesion vs sliding
        float Fx = 0.f, Fy = 0.f;

        if (S > 1e-12f && Fz > 0.f)
        {
            // Slip direction unit vector
            float c = tx / S;
            float s = ty / S;

            // Directional peak friction along slip direction
            float muX_peak = mTireParams.peakFrictionCoeffLong;
            float muY_peak = mTireParams.peakFrictionCoeffLat;
            float muDir_peak = std::sqrt((muX_peak * c) * (muX_peak * c) + (muY_peak * s) * (muY_peak * s));

            // 2-D adhesion–sliding boundary
            float xs = 2.0f * a * (muDir_peak * p) / S - a;
            xs = glm::clamp(xs, -a, a);

            // Adhesion contributions over [-a, xs] with linear shear ~(x+a)/(2a)
            float factor = (xs + a);
            float Fx_adh = (2.0f * b) * (kxA * slipRatio) * (factor * factor / (4.0f * a));
            float Fy_adh = (2.0f * b) * (kyA * tanSlipAngle) * (factor * factor / (4.0f * a));

            // Nonlinear grip (sliding part only)
            float slidingFraction = (a - xs) / (2.0f * a);

            float muX_slide = mTireParams.slidingFrictionFactorLong * muX_peak;
            float muY_slide = mTireParams.slidingFrictionFactorLat * muY_peak;

            float muX_eff = muX_slide + (muX_peak - muX_slide) *
                std::pow(1.0f - slidingFraction, mTireParams.slidingFrictionFalloffExponentLong);

            float muY_eff = muY_slide + (muY_peak - muY_slide) *
                std::pow(1.0f - slidingFraction, mTireParams.slidingFrictionFalloffExponentLat);

            // Effective friction along the current slip direction
            float muDir_eff = std::sqrt((muX_eff * c) * (muX_eff * c) +
                (muY_eff * s) * (muY_eff * s));

            // Sliding contributions over [xs, +a]
            float span = (a - xs);
            float Fx_sl = (2.0f * b) * (muDir_eff * p * c * span);
            float Fy_sl = (2.0f * b) * (muDir_eff * p * s * span);

            // Forces oppose slip
            Fx = (Fx_adh + Fx_sl);
            Fy = -(Fy_adh + Fy_sl);
        }

        // Compute max longitudinal force from available torque
        float effectiveBrakeTorque = 0.0f;
        if (mBrakeTorque > 0.0f)
        {
            float spinSign = (std::fabs(mWheelAngularVelocity) > 1e-3f)
                ? glm::sign(mWheelAngularVelocity)
                : glm::sign(Vx); // oppose rolling if w ~ 0
            effectiveBrakeTorque = -spinSign * std::abs(mBrakeTorque);
        }

        float torqueAvailable = mDriveTorque + effectiveBrakeTorque;
        if (std::abs(torqueAvailable) > 0.01f)
        {
            float FxCap = std::abs(torqueAvailable) / mTireParams.tireRadius;
            if (glm::sign(Fx) == glm::sign(torqueAvailable))
            {
                Fx = glm::clamp(Fx, -FxCap, FxCap); // clamp only when aligned
            }
            // if Fx is trying to UN-lock the wheel, don't clamp it
        }

        glm::vec3 forceWorld = projForward * Fx + projSide * Fy;
        mCarRb->ApplyForce(forceWorld, mSuspension->GetContactPoint());

        // Apply rolling resistance
        glm::vec3 rollingResistanceDir = -projForward * glm::sign(Vx);
        glm::vec3 rollingResistanceForce = rollingResistanceDir * mTireParams.rollingResistance * Fz;
        mCarRb->ApplyForce(rollingResistanceForce, mSuspension->GetContactPoint());

        // Compute and apply wheel angular acceleration
        float roadTorque = -Fx * mTireParams.tireRadius;
        float netTorque = mDriveTorque + effectiveBrakeTorque + roadTorque;

        float inertia = 0.5f * mTireParams.wheelMass * mTireParams.tireRadius * mTireParams.tireRadius;
        float Vref = std::sqrt(Vx * Vx + (mWheelAngularVelocity * mTireParams.tireRadius) * (mWheelAngularVelocity * mTireParams.tireRadius) + 0.25f * 0.25f);
        // Blend factor: ~1 at speed, ~0 near standstill
        float v0 = 1.0f; // m/s, tune 0.5–2.0
        float g = std::fabs(Vx) / (std::fabs(Vx) + v0);

        // Effective small-slip slope used only for the implicit stabilizer
        float Kk_eff = 0.3f * Cx * g;   // 0.2–0.5 × Cx is typical; g kills it near 0 speed

        // Closed-form backward-Euler
        float A = 1.0f + (dt / inertia) * ((mTireParams.tireRadius * mTireParams.tireRadius * Kk_eff) / Vref);
        float B = mWheelAngularVelocity + (dt / inertia) * (mDriveTorque + effectiveBrakeTorque + (mTireParams.tireRadius * Kk_eff / Vref) * Vx);

        mWheelAngularVelocity = B / A;

        //std::cout << GetEntity()->GetTag() << " slip ratio: " << slipRatio << std::endl;

        // Clear torques for next frame
        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;

        //mGripUsage = glm::sqrt(FxRaw * FxRaw + FyRaw * FyRaw) / Fmax;

        //std::cout << GetEntity()->GetTag() << " Grip Usage: " << mGripUsage << std::endl;

        // Set tire screech audio based on slip conditions
        float tireScreechVolume = 0.0f;
        if (glm::length(carVel) > 5)
        {
            if (slipRatio > 0.0f)
                tireScreechVolume = glm::clamp(slipRatio, 0.f, 1.f);
            else
                tireScreechVolume = glm::clamp(-slipRatio * 3, 0.f, 1.f);
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