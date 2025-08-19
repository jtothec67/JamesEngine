#pragma once

#include "JamesEngine/JamesEngine.h"

namespace JamesEngine
{

	struct EngineParams
	{
		float maxRPM;
		float idleRPM;
		std::vector<float> gearRatios;
		std::vector<std::pair<float, float>> torqueCurve;
		float finalDrive;
		float drivetrainEfficiency;

		float bitePointStart = 0.35f;
		float bitePointEnd = 0.65f;

		float freeRevRate = 12000.0f;
		float decayRate = 3000.0f;

		float engineBrakeBaseK = 80.0f;
		float throttleIdleThreshold = 0.05f;
	};

	class Engine
	{
	public:
        void SetEngineParams(const EngineParams& params) { mParams = params; }

        void SetGear(int gear) { mCurrentGear = glm::clamp(gear, 1, (int)std::max<size_t>(1, mParams.gearRatios.size())); }
		int GetCurrentGear() { return mCurrentGear; }

		void SetAutoClutchEnabled(bool enabled) { mAutoClutchEnabled = enabled; }
		float GetClutch() { return mClutch; }

        void EngineUpdate(float throttle, float clutch, float wheelRPM, float dt);

        float GetRPM() { return mCurrentRPM; }
        float GetWheelTorque();

    private:
        EngineParams mParams;

        int mCurrentGear = 1;
        float mCurrentRPM = 0.0f;

		enum class LaunchState { PreLaunch, Hold, Release };
		bool mAutoClutchEnabled = true;
		LaunchState mLaunchState = LaunchState::PreLaunch;

        // cached inputs
        float mThrottle = 0.0f;
        float mClutch = 1.0f;
    };

}