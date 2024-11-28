#include "Input.h"

namespace JamesEngine
{

	Input::Input()
	{
		mKeyboard = std::make_shared<Keyboard>();
	}

	bool Input::Update()
	{
		mKeyboard->Update();

		SDL_Event event = {};
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				return false;
			}
			else if (event.type == SDL_KEYDOWN)
			{
				mKeyboard->keys.push_back(event.key.keysym.sym);
				mKeyboard->pressedKeys.push_back(event.key.keysym.sym);
			}
			else if (event.type == SDL_KEYUP)
			{
				mKeyboard->KeyBeenReleased(event.key.keysym.sym);
				mKeyboard->releasedKeys.push_back(event.key.keysym.sym);
			}
		}

		return true;
	}

}