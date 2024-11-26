#pragma once

#include "Keyboard.h"

namespace JamesEngine
{

	class Input
	{
	public:
		bool Update();

	private:
		Keyboard mKeyboard;
	};

}