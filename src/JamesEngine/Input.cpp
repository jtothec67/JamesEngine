#include "Input.h"

namespace JamesEngine
{

	bool Input::Update()
	{
		mKeyboard.Update();

		SDL_Event event = {};
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				return false;
			}
			
			if (event.type == SDL_KEYDOWN)
			{
				//mKeyboard.keys.emplace_back(event.key.keysym.sym);
				//mKeyboard.pressedKeys.emplace_back(event.key.keysym.sym);
			}

			if (event.type == SDL_KEYUP)
			{
				//mKeyboard.keys.emplace_back(event.key.keysym.sym);
				//mKeyboard.pressedKeys.emplace_back(event.key.keysym.sym);
			}
		}

		return true;
	}

}