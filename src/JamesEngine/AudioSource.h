#pragma once

#include "Component.h"
#include "Sound.h"

#ifdef _DEBUG

#include "Renderer/Model.h"
#include "Renderer/Shader.h"

#endif // _DEBUG

#include <AL/al.h>

namespace JamesEngine
{

	class AudioSource : public Component
	{
	public:
		AudioSource() { alGenSources(1, &mSourceId); }
		~AudioSource();

		void OnTick();

#ifdef _DEBUG
		void OnRender();
#endif // _DEBUG

		void SetSound(std::shared_ptr<Sound> _sound) { mSound = _sound; alSourcei(mSourceId, AL_BUFFER, mSound->mBufferId); }

		bool IsPlaying();
		void Play();

		void SetOffset(glm::vec3 _offset) { mOffset = _offset; }

		void SetPitch(float _pitch) { mPitch = _pitch; }
		void SetGain(float _gain) { mGain = _gain; }
		void SetLooping(bool _looping) { mLooping = _looping; }

		void SetReferenceDistance(float _referenceDistance) { mReferenceDistance = _referenceDistance; }
		void SetMaxDistance(float _maxDistance) { mMaxDistance = _maxDistance; }
		void SetRolloffFactor(float _rollOffFactor) { mRollOffFactor = _rollOffFactor; }

	private:
		std::shared_ptr<Sound> mSound = nullptr;

		glm::vec3 mOffset = glm::vec3(0.f, 0.f, 0.f);

		ALenum mFormat = 0;
		ALsizei mFrequency = 0;

		ALuint mBufferId = 0;
		ALuint mSourceId = 0;

		bool mLooping = false;

		float mPitch = 1.f;
		float mGain = 1.f;

		float mReferenceDistance = 10.f;
		float mMaxDistance = 100.f;
		float mRollOffFactor = 1.f;

#ifdef _DEBUG

		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/sphere.obj");
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../assets/shaders/OutlineShader.vert", "../assets/shaders/OutlineShader.frag");

#endif // _DEBUG
	};

}