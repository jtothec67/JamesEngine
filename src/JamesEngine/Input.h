#pragma once

#include "Keyboard.h"

#include <memory>

namespace JamesEngine
{

	class Input
	{
	public:
		Input();

		bool Update();

		std::shared_ptr<Keyboard> GetKeyboard() { return mKeyboard; }

	private:
		std::shared_ptr<Keyboard> mKeyboard;
	};

}