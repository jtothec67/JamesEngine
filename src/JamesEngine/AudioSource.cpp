#include "AudioSource.h"

#include "Transform.h"
#include "Entity.h"
#include "Core.h"
#include "Camera.h"

#ifdef _DEBUG

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#endif // _DEBUG

namespace JamesEngine
{

	AudioSource::~AudioSource()
	{
		alDeleteSources(1, &mSourceId);
	}

	void AudioSource::OnTick()
	{
		std::shared_ptr<Core> core = GetEntity()->GetCore();
		glm::vec3 cameraPosition = core->GetCamera()->GetPosition();
		alListener3f(AL_POSITION, cameraPosition.x, cameraPosition.y, cameraPosition.z);
		alSource3f(mSourceId, AL_POSITION, GetPosition().x + mOffset.x, GetPosition().y + mOffset.y, GetPosition().z + mOffset.z);

		if (glm::distance(GetPosition() + mOffset, cameraPosition) > mMaxDistance)
		{
			alSourcef(mSourceId, AL_GAIN, 0.0f);
		}
		else
		{
			alSourcef(mSourceId, AL_GAIN, mGain);
		}

		alSourcef(mSourceId, AL_PITCH, mPitch);
		alSourcef(mSourceId, AL_REFERENCE_DISTANCE, mReferenceDistance);
		alSourcef(mSourceId, AL_MAX_DISTANCE, mMaxDistance);
		alSourcef(mSourceId, AL_ROLLOFF_FACTOR, mRollOffFactor);

		if (mLooping && !IsPlaying())
			Play();
	}

#ifdef _DEBUG
	void AudioSource::OnRender()
	{
		std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();

		mShader->uniform("projection", camera->GetProjectionMatrix());

		mShader->uniform("view", camera->GetViewMatrix());

		glm::mat4 mModelMatrix = glm::mat4(1.f);
		mModelMatrix = glm::translate(mModelMatrix, GetPosition() + mOffset);
		mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mMaxDistance, mMaxDistance, mMaxDistance));

		mShader->uniform("model", mModelMatrix);

		mShader->uniform("outlineWidth", 1.f);

		mShader->uniform("outlineColor", glm::vec3(0, 0, 1));

		glDisable(GL_DEPTH_TEST);

		mShader->drawOutline(mModel.get());

		glEnable(GL_DEPTH_TEST);


		glm::mat4 mModelMatrix2 = glm::mat4(1.f);
		mModelMatrix2 = glm::translate(mModelMatrix2, GetPosition() + mOffset);
		mModelMatrix2 = glm::scale(mModelMatrix2, glm::vec3(mReferenceDistance, mReferenceDistance, mReferenceDistance));

		mShader->uniform("model", mModelMatrix2);

		glDisable(GL_DEPTH_TEST);

		mShader->drawOutline(mModel.get());

		glEnable(GL_DEPTH_TEST);
	}
#endif // _DEBUG

	bool AudioSource::IsPlaying()
	{
		int state = 0; 
		alGetSourcei(mSourceId, AL_SOURCE_STATE, &state); 
		if (state == AL_PLAYING)
			return true;
		
		return false;
	}

	void AudioSource::Play()
	{
		alSourcePlay(mSourceId);
	}

}