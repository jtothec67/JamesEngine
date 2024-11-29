#include "Input.h"

#include <iostream>

namespace JamesEngine
{

	Input::Input()
	{
		mKeyboard = std::make_shared<Keyboard>();
		mMouse = std::make_shared<Mouse>();
	}

	void Input::Update()
	{
		mKeyboard->Update();
	}

	void Input::HandleInput(const SDL_Event& _event)
	{
		if (_event.type == SDL_KEYDOWN)
		{
			mKeyboard->mKeys.push_back(_event.key.keysym.sym);
			mKeyboard->mKeysDown.push_back(_event.key.keysym.sym);
		}
		else if (_event.type == SDL_KEYUP)
		{
			mKeyboard->KeyBeenReleased(_event.key.keysym.sym);
			mKeyboard->mKeysUp.push_back(_event.key.keysym.sym);
		}
		else if (_event.type == SDL_MOUSEBUTTONDOWN)
		{
			mMouse->mButtons.push_back(_event.button.button);
			mMouse->mButtonsDown.push_back(_event.button.button);
		}
		else if (_event.type == SDL_MOUSEBUTTONUP)
		{
			mMouse->ButtonBeenReleased(_event.button.button);
			mMouse->mButtonsUp.push_back(_event.button.button);
		}
		else if (_event.type == SDL_MOUSEMOTION)
		{
			mMouse->mXpos = _event.key.keysym.sym;
			mMouse->mYpos = _event.key.keysym.sym;
		}
	}

}