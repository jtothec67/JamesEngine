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
            const float idleStallRPM = mParams.idleRPM + 500.0f;
            const float bitePoint = 0.5f;
            const float releaseStartRPM = 2000.0f;
            const float releaseEndRPM = 3000.0f;
            const float throttleThreshold = 0.05f;

            const bool throttlePressed = (mThrottle > throttleThreshold);
            const bool stalled = (mCurrentRPM < idleStallRPM);

            // State transitions
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
                if (!throttlePressed && mCurrentRPM < idleStallRPM + 100.0f)
                    mLaunchState = LaunchState::PreLaunch;
                break;
            }

            // Behavior
            if (stalled)
            {
                mClutch = 0.0f; // Anti-stall: fully open
            }
            else if (!throttlePressed)
            {
                mClutch = 1.0f; // Fully engaged when not launching
            }
            else
            {
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