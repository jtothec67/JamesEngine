#pragma once

#include "JamesEngine/JamesEngine.h"

namespace JamesEngine
{

	struct EngineParams
	{
		float maxRPM;
		float idleRPM;
		std::vector<float> gearRatios;
		float finalDrive;
		float drivetrainEfficiency;

		float bitePointStart = 0.35f;
		float bitePointEnd = 0.65f;

		std::vector<std::pair<float, float>> torqueCurve;
	};

	class Engine : public Component
	{
	public:
		void OnAlive();

		void EngineUpdate(float _throttleInput, float _clutchEngagement, float _wheelRPM);

		void SetEngineParams(const EngineParams& params) { mParams = params; }

		void SetEngineAudioSource(std::shared_ptr<AudioSource> audioSource) { engineAudioSource = audioSource; }
		void SetEngineSound(std::shared_ptr<Sound> sound) { engineSound = sound; }

	private:
		EngineParams mParams;

		std::shared_ptr<AudioSource> engineAudioSource;
		std::shared_ptr<Sound> engineSound;
	};
}