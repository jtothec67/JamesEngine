#include "Engine.h"

namespace JamesEngine
{

    void Engine::EngineUpdate(float _throttle, float _clutch, float _wheelRPM, float _dt)
    {
        mThrottle = _throttle;
        mClutch = _clutch;

        // calculate target engine rpm from wheels
        float gearRatio = mParams.gearRatios.empty() ? 1.0f : mParams.gearRatios[mCurrentGear - 1];
        float targetEngineRPM = _wheelRPM * gearRatio * mParams.finalDrive;

        // Auto-clutch logic
        if (mAutoClutchEnabled)
        {
            const float throttleThreshold = 0.05f;
            const float bitePoint = 0.5f;
            const float releaseStartRPM = 2000.0f;
            const float releaseEndRPM = 3000.0f;

            // Anti-stall hysteresis around idle
            const float stallEngageRPM = mParams.idleRPM + 150.0f; // enter anti-stall below this
            const float stallReleaseRPM = mParams.idleRPM + 50.0f;  // leave anti-stall above this

            // Off-throttle clutch target based on slip (prevents re-stall but allows engine braking)
            const float openSlipRPM = 800.0f;  // slip at which we fully open (0 clutch)
            const float closedSlipRPM = 200.0f;  // slip at which we can fully close (1 clutch)

            const bool throttlePressed = (mThrottle > throttleThreshold);

            if (!mAntiStallActive)
            {
                // Enter anti-stall only when BOTH engine and wheel-imposed RPMs are very low
                if (mCurrentRPM < stallEngageRPM && targetEngineRPM < stallEngageRPM)
                    mAntiStallActive = true;
            }
            else
            {
                // Exit anti-stall when EITHER rises (wheels unlock or engine speeds up)
                if (mCurrentRPM > stallReleaseRPM || targetEngineRPM > stallReleaseRPM)
                    mAntiStallActive = false;
            }

            switch (mLaunchState)
            {
            case LaunchState::PreLaunch:
                if (throttlePressed) mLaunchState = LaunchState::Hold;
                break;
            case LaunchState::Hold:
                if (targetEngineRPM >= releaseStartRPM) mLaunchState = LaunchState::Release;
                break;
            case LaunchState::Release:
                // Reset when driver lifts and revs drop back near idle
                if (!throttlePressed && mCurrentRPM < (mParams.idleRPM + 100.0f))
                    mLaunchState = LaunchState::PreLaunch;
                break;
            }

            if (mAntiStallActive)
            {
                // Protect the engine when wheels are locked / near-zero
                mClutch = 0.0f; // fully open
            }
            else if (!throttlePressed)
            {
                // OFF-THROTTLE: engage clutch to get engine braking, but avoid re-stall using slip
                const float slipRPM = mCurrentRPM - targetEngineRPM;  // +ve: engine faster than wheels
                // Map slip to a 0..1 "openness" factor, then invert to get target clutch
                float t = glm::clamp((std::abs(slipRPM) - closedSlipRPM) / (openSlipRPM - closedSlipRPM), 0.0f, 1.0f);
                float targetClutch = 1.0f - t; // 1 at low slip -> strong engine braking; fades open as slip grows
                // Only increase toward target (prevents chatter if current mClutch is already higher)
                mClutch = std::max(mClutch, targetClutch);
            }
            else
            {
                // Throttle pressed: use your launch state machine
                switch (mLaunchState)
                {
                case LaunchState::PreLaunch:
                    mClutch = 0.0f;
                    break;

                case LaunchState::Hold:
                    // Gradually increase clutch up to bite point
                    mClutch = std::min(mClutch + _dt * 2.0f, bitePoint);
                    break;

                case LaunchState::Release:
                {
                    // Linearly release from bitePoint to full engagement as targetRPM rises 2000->3000
                    float t = glm::clamp((targetEngineRPM - releaseStartRPM) / (releaseEndRPM - releaseStartRPM), 0.0f, 1.0f);
                    mClutch = glm::mix(bitePoint, 1.0f, t);
                    break;
                }
                }
            }

            // Clamp final clutch
            mClutch = glm::clamp(mClutch, 0.0f, 1.0f);
        }

        float throttle = glm::clamp(mThrottle, 0.0f, 1.0f);

        // Simulate free-rev RPM (clutch fully disengaged)
        float freeRevRPM = mCurrentRPM;
        if (throttle > 0.0f)
            freeRevRPM += throttle * mParams.freeRevRate * _dt;
        else
            freeRevRPM -= mParams.decayRate * _dt;

        // Clamp freeRevRPM
        freeRevRPM = glm::clamp(freeRevRPM, mParams.idleRPM, mParams.maxRPM);

        // Simulate driven RPM (clutch fully engaged)
        float drivenRPM = glm::mix(mCurrentRPM, targetEngineRPM, _dt * 10.0f);

        // Define clutch bite point behavior
        float clutchTorqueFactor = 0.0f;
        if (mClutch < mParams.bitePointStart)
            clutchTorqueFactor = 0.0f;
        else if (mClutch < mParams.bitePointEnd)
            clutchTorqueFactor = (mClutch - mParams.bitePointStart) / (mParams.bitePointEnd - mParams.bitePointStart); // linear ramp
        else
            clutchTorqueFactor = 1.0f;

        // Blend current RPM
        mCurrentRPM = glm::mix(freeRevRPM, drivenRPM, clutchTorqueFactor);

        // Clamp result
        mCurrentRPM = glm::clamp(mCurrentRPM, mParams.idleRPM, mParams.maxRPM);
    }

    float Engine::GetWheelTorque()
    {
        // sample torque curve at current rpm
        float engineTorque = 0.0f;
        if (!mParams.torqueCurve.empty())
        {
            if (mCurrentRPM <= mParams.torqueCurve.front().first)
                engineTorque = mParams.torqueCurve.front().second;
            else if (mCurrentRPM >= mParams.torqueCurve.back().first)
                engineTorque = mParams.torqueCurve.back().second;
            else
            {
                for (size_t i = 0; i + 1 < mParams.torqueCurve.size(); ++i)
                {
                    float rpm1 = mParams.torqueCurve[i].first;
                    float tq1 = mParams.torqueCurve[i].second;
                    float rpm2 = mParams.torqueCurve[i + 1].first;
                    float tq2 = mParams.torqueCurve[i + 1].second;
                    if (mCurrentRPM >= rpm1 && mCurrentRPM <= rpm2)
                    {
                        float t = (mCurrentRPM - rpm1) / (rpm2 - rpm1);
                        engineTorque = tq1 + (tq2 - tq1) * t;
                        break;
                    }
                }
            }
        }

        // apply throttle
        engineTorque *= mThrottle;

        // cut at redline
        if (mCurrentRPM >= mParams.maxRPM)
            engineTorque = 0.0f;

        // engine braking
        if (mThrottle < mParams.throttleIdleThreshold)
        {
            float norm = (mCurrentRPM - mParams.idleRPM) / (mParams.maxRPM - mParams.idleRPM);
            norm = glm::clamp(norm, 0.0f, 1.0f);
            float rpmCurve = norm * norm;
            float gearRatio = mParams.gearRatios.empty() ? 1.0f : mParams.gearRatios[mCurrentGear - 1];
            engineTorque += -rpmCurve * mParams.engineBrakeBaseK * gearRatio;
        }

        // convert to wheel torque
        float gearRatio = mParams.gearRatios.empty() ? 1.0f : mParams.gearRatios[mCurrentGear - 1];
        float wheelTorque = engineTorque * gearRatio * mParams.finalDrive * mParams.drivetrainEfficiency;

        // scale by clutch bite
        float bite = 0.0f;
        if (mClutch <= mParams.bitePointStart) bite = 0.0f;
        else if (mClutch >= mParams.bitePointEnd) bite = 1.0f;
        else bite = (mClutch - mParams.bitePointStart) / (mParams.bitePointEnd - mParams.bitePointStart);

        wheelTorque *= bite;

        return wheelTorque;
    }

}