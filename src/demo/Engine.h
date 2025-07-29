#pragma once

#include "JamesEngine/JamesEngine.h"

namespace JamesEngine
{

	struct EngineParams
	{
		float peakPowerKW;
		float maxRPM;
		float idleRPM;
		std::vector<float> gearRatios;
		float finalDrive;
		float drivetrainEfficiency;
	};

	class Engine : public Component
	{
	public:
		void OnAlive();

		void EngineUpdate(float _throttleInput);

		void SetEngineParams(const EngineParams& params) { mParams = params; }

		void SetEngineAudioSource(std::shared_ptr<AudioSource> audioSource) { engineAudioSource = audioSource; }
		void SetEngineSound(std::shared_ptr<Sound> sound) { engineSound = sound; }

	private:
		EngineParams mParams;

		std::shared_ptr<AudioSource> engineAudioSource;
		std::shared_ptr<Sound> engineSound;
	};
}