#pragma once

#include "Resource.h"

#include <AL/al.h>
#include <AL/alc.h>

namespace JamesEngine
{

	class AudioSource;

	class Sound : public Resource
	{
	public:
		void OnLoad() { }

	private:
		friend class AudioSource;

		ALuint mBufferId = 0;
	};

}