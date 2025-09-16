#include "Tire.h"

#include "Suspension.h"
#include "../JamesEngine/Timer.h"

#include <iomanip>

namespace JamesEngine
{

    void Tire::OnAlive()
    {
        mInertia = 0.5f * mTireParams.wheelMass * mTireParams.tireRadius * mTireParams.tireRadius;

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
		//ScopedTimer timer("Tire::OnFixedTick");
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

        // Compute vertical load from suspension compression and weight transfer
        float Fz = mSuspension->GetForce();

        // Compute max longitudinal force from available torque
        float effectiveBrakeTorque = 0.0f;
        if (mBrakeTorque > 0.0f)
        {
            float spinSign = (std::fabs(mWheelAngularVelocity) > 1e-3f)
                ? glm::sign(mWheelAngularVelocity)
                : glm::sign(Vx); // oppose rolling if w ~ 0
            effectiveBrakeTorque = -spinSign * std::abs(mBrakeTorque);
        }

        // === Improved fully-implicit wheel integration (drop-in) ===
        const float R = mTireParams.tireRadius;
        const float cVisc = 0.02f;   // small hub viscous loss (0.0–0.05 ok)
        const int iters = 1;       // Newton iterations (usually 1–3 suffice, 8 is safe)

        bool stickActive = false;
        float Fx_static = 0.0f;

        // --------- (1) Static "stick" at crawl (no creep when torque < static friction) ---------
        const float v_stat = 0.30f;                                 // m/s threshold for ground speed
        const float w_stat = 0.25f;                                 // rad/s threshold for wheel spin
        const float mu_s = mTireParams.peakFrictionCoeffLong;     // use a dedicated static mu if you have one

        float tau_app = mDriveTorque + effectiveBrakeTorque;        // actuator torque at hub (no tire yet)
        float T_static = mu_s * Fz * R;                             // max tire torque in stick

        bool nearStatic = (std::fabs(Vx) < v_stat) && (std::fabs(mWheelAngularVelocity) < w_stat);

        // small hysteresis keeps it from chattering at threshold
        const float breakScale = 1.02f; // must exceed static by ~2% to break free

        if (nearStatic && std::fabs(tau_app) <= T_static)
        {
            // Hold contact: wheel speed matches ground and tire transmits hub torque statically
            mWheelAngularVelocity = (R > 0.0f) ? (Vx / R) : 0.0f;
            stickActive = true;

            // Transmit hub torque into longitudinal force (equal and opposite road torque)
            Fx_static = tau_app / R;   // road torque = -R*Fx = -tau_app
        }
        else if (nearStatic && std::fabs(tau_app) < breakScale * T_static)
        {
            // Optional gentle stick band to avoid instant chatter just above T_static
            mWheelAngularVelocity = (R > 0.0f) ? (Vx / R) : 0.0f;
            stickActive = true;
        }
        else
        {
            // --------- fully implicit backward–Euler (your Newton loop) ---------
            float omega_n = mWheelAngularVelocity;
            float omega = omega_n;

            auto FxAt = [&](float om) -> float { return BrushTireModel(Vx, Vy, om, Fz).x; };

            for (int i = 0; i < iters; ++i)
            {
                float Fx0 = FxAt(omega);
                float g_eq = omega - omega_n - (dt / mInertia) * (tau_app - R * Fx0 - cVisc * omega);

                float dW = glm::max(0.25f, 0.1f + 0.05f * std::fabs(omega));   // central diff
                float Fx_p = FxAt(omega + dW);
                float Fx_m = FxAt(omega - dW);
                float dFx_domega = (Fx_p - Fx_m) / (2.0f * dW);
                float dTau_domega = -R * dFx_domega;

                float dg = 1.0f - (dt / mInertia) * (dTau_domega - cVisc);
                if (std::fabs(dg) < 1e-8f) break;

                float step = g_eq / dg;
                step = glm::clamp(step, -20.0f, 20.0f);

                // tiny line search to prevent overshoot
                float omega_try = omega - step;
                for (int ls = 0; ls < 3; ++ls)
                {
                    float Fx_try = FxAt(omega_try);
                    float g_try = omega_try - omega_n - (dt / mInertia) * (tau_app - R * Fx_try - cVisc * omega_try);
                    if (std::fabs(g_try) <= 0.9f * std::fabs(g_eq)) break;
                    step *= 0.5f;
                    omega_try = omega - step;
                }
                omega -= step;
                if (std::fabs(step) < 1e-4f) break;
            }

            mWheelAngularVelocity = omega;
        }

        glm::vec2 tireForce = BrushTireModel(Vx, Vy, mWheelAngularVelocity, Fz);
        float Fx = tireForce.x;
        if (stickActive) Fx = Fx_static;  // transmit hub torque while k~0
        float Fy = tireForce.y;

        glm::vec3 forceWorld = projForward * Fx + projSide * Fy;
        mCarRb->ApplyForce(forceWorld, mSuspension->GetContactPoint());

        // Apply rolling resistance
        glm::vec3 rollingResistanceDir = -projForward * glm::sign(Vx);
        glm::vec3 rollingResistanceForce = rollingResistanceDir * mTireParams.rollingResistance * Fz;
        mCarRb->ApplyForce(rollingResistanceForce, mSuspension->GetContactPoint());

        // Clear torques for next frame
        mDriveTorque = 0.0f;
        mBrakeTorque = 0.0f;

        //mGripUsage = glm::sqrt(FxRaw * FxRaw + FyRaw * FyRaw) / Fmax;

        //std::cout << GetEntity()->GetTag() << " Grip Usage: " << mGripUsage << std::endl;

        float wheelCircumferentialSpeed = mWheelAngularVelocity * mTireParams.tireRadius;

        float slipRatioDenom = glm::max(std::fabs(Vx), 0.5f);
        float slipRatio = (wheelCircumferentialSpeed - Vx) / slipRatioDenom;
        slipRatio = glm::clamp(slipRatio, -3.0f, 3.0f);

        //std::cout << GetEntity()->GetTag() << " slip ratio: " << slipRatio << std::endl;

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

    glm::vec2 Tire::BrushTireModel(float Vx, float Vy, float omega, float Fz)
    {
        // Determine tire stiffness and maximum friction force
        float FzRef = mTireParams.loadSensitivityRef;
        float loadScale = glm::max(Fz, 1.0f) / glm::max(FzRef, 1.0f);

        // Stiffness with load sensitivity
        float Cx = mTireParams.longStiffCoeff * FzRef * std::pow(loadScale, mTireParams.longStiffExp);
        float Cy = mTireParams.latStiffCoeff * FzRef * std::pow(loadScale, mTireParams.latStiffExp);

        float R = mTireParams.tireRadius;

        float V_wheel = omega * R;

        // Slip inputs
        float slipRatioDenom = glm::max(std::fabs(Vx), 0.5f);
        float slipRatio = (V_wheel - Vx) / slipRatioDenom;
        slipRatio = glm::clamp(slipRatio, -3.0f, 3.0f);

        float slipAngleDenom = glm::max(std::fabs(Vx), 1.0f);
        float tanSlipAngle = Vy / slipAngleDenom;

        // Geometric half-dimensions of the footprint
        float b = mTireParams.contactHalfLengthY;

        float FzAtMax = glm::max(mTireParams.refMaxLoadContactHalfLengthX, 1.0f);
        float t = glm::clamp(Fz / FzAtMax, 0.0f, 1.0f);
        float a = mTireParams.maxContactHalfLengthX * t;  // 0 at 0 load -> max at ref load

        // Convert to per-area bristle stiffness (preserves small-slip slopes)
        float kxA = Cx / (2.0f * a * b);
        float kyA = Cy / (2.0f * a * b);

        // Uniform pressure over rectangular patch
        float p = Fz / (4.0f * a * b);

        // Build combined shear slope (per-area)
        float tx = kxA * slipRatio;
        float ty = kyA * tanSlipAngle;
        float S = glm::sqrt(tx * tx + ty * ty);

        float Fx = 0.f, Fy = 0.f;

        if (S > 1e-12f && Fz > 0.f)
        {
            // Slip direction unit vector
            float c = tx / S;
            float s = ty / S;

            // Directional peak friction along slip direction
            float muX_peak = mTireParams.peakFrictionCoeffLong;
            float muY_peak = mTireParams.peakFrictionCoeffLat;
            float muDir_pk = std::sqrt((muX_peak * c) * (muX_peak * c) + (muY_peak * s) * (muY_peak * s));

            // 2-D adhesion–sliding boundary
            float xs = 2.0f * a * (muDir_pk * p) / S - a;
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

            // Compute final forces
            Fx = Fx_adh + Fx_sl;
            Fy = -(Fy_adh + Fy_sl);
        }

        return glm::vec2(Fx, Fy);
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