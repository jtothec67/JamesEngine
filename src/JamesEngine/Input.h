#pragma once

#include "Keyboard.h"
#include "Mouse.h"

#include <SDL2/sdl.h>
#include <memory>

namespace JamesEngine
{
	class Core;

	class Input
	{
	public:
		Input();

		void Update();

		std::shared_ptr<Keyboard> GetKeyboard() { return mKeyboard; }
		std::shared_ptr<Mouse> GetMouse() { return mMouse; }

	private:
		friend class Core;

		void HandleInput(const SDL_Event& _event);

		std::shared_ptr<Keyboard> mKeyboard;
		std::shared_ptr<Mouse> mMouse;
	};

}