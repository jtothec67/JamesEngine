#include "AudioSource.h"
#include "Transform.h"

#include <stdexcept>

namespace JamesEngine
{

	AudioSource::~AudioSource()
	{
		alDeleteSources(1, &mSourceId);
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

}