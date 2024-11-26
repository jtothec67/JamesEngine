#pragma once

#include <SDL2/sdl.h>

#include <vector>

namespace JamesEngine
{
	

	class Keyboard
	{
	public:
		Keyboard() {}
		~Keyboard() {}

		void Update();

		bool IsKey(int _key);
		bool IsKeyDown(int _key);
		bool IsKeyUp(int _key);

	private:
		friend class Input;

		std::vector<int> keys;
		std::vector<int> pressedKeys;
		std::vector<int> releasedKeys;
	};

}