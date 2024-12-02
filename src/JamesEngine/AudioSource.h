#pragma once

#include "Component.h"
#include "Sound.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <exception>
#include <vector>
#include <string>

namespace JamesEngine
{

	class AudioSource : public Component
	{
	public:
		AudioSource() { }
		~AudioSource();

		void SetSound(std::string _path);

		void SetPitch(float _pitch) { alSourcef(mSourceId, AL_PITCH, _pitch); }
		void SetVolume(float _volume) { alSourcef(mSourceId, AL_GAIN, _volume); }
		void SetLooping(bool _looping) { mLooping = _looping; }

		bool IsPlaying();

		void OnTick();

		void Play();

	private:
		void Load_ogg(std::string _path, std::vector<unsigned char>& _data);

		std::shared_ptr<Sound> mSound = nullptr;

		ALenum mFormat = 0;
		ALsizei mFrequency = 0;

		ALuint mBufferId = 0;
		ALuint mSourceId = 0;

		bool mLooping = false;
	};

}