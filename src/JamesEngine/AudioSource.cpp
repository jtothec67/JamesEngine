#include "AudioSource.h"

#include "Transform.h"

#include "stb_vorbis.c"

#include <stdexcept>

namespace JamesEngine
{

	AudioSource::AudioSource()
	{
		
	}

	AudioSource::~AudioSource()
	{
		alDeleteSources(1, &mSourceId);
		alDeleteBuffers(1, &mBufferId);
	}

	void AudioSource::SetSound(std::string _path)
	{
		alDeleteSources(1, &mSourceId);
		alDeleteBuffers(1, &mBufferId);

		Load_ogg(_path);

		alGenBuffers(1, &mBufferId);

		alBufferData(mBufferId, mFormat, &mBufferData.at(0),
			static_cast<ALsizei>(mBufferData.size()), mFrequency);

		alGenSources(1, &mSourceId);

		alSourcei(mSourceId, AL_BUFFER, mBufferId);
	}

	bool AudioSource::IsPlaying()
	{
		int state = 0; 
		alGetSourcei(mSourceId, AL_SOURCE_STATE, &state); 
		if (state == AL_PLAYING)
			return true;
		
		return false;
	}

	void AudioSource::OnTick()
	{
		alSource3f(mSourceId, AL_POSITION, GetTransform()->GetPosition().x, GetTransform()->GetPosition().y, GetTransform()->GetPosition().z);
		
		if (mLooping && !IsPlaying())
			Play();
	}

	void AudioSource::Play()
	{
		alSourcePlay(mSourceId);
	}

	void AudioSource::Load_ogg(std::string _path)
	{
		int channels = 0;
		int sampleRate = 0;
		short* output = NULL;

		size_t samples = stb_vorbis_decode_filename(_path.c_str(),
			&channels, &sampleRate, &output);

		if (samples == -1)
		{
			throw std::runtime_error("Failed to open file '" + _path + "' for decoding");
		}

		// Record the format required by OpenAL
		if (channels < 2)
		{
			mFormat = AL_FORMAT_MONO16;
		}
		else
		{
			mFormat = AL_FORMAT_STEREO16;
		}

		// Copy (# samples) * (1 or 2 channels) * (16 bits == 2 bytes == short)
		mBufferData.resize(samples * channels * sizeof(short));
		memcpy(&mBufferData.at(0), output, mBufferData.size());

		// Record the sample rate required by OpenAL
		mFrequency = sampleRate;

		// Clean up the read data
		free(output);
	}

}