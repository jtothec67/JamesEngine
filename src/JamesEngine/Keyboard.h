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

		void KeyBeenReleased(int _key)
		{
			for (int i = 0; i < keys.size(); ++i)
			{
				if (keys[i] == _key)
				{
					keys.erase(keys.begin() + i);
				}
			}
		}

		std::vector<int> keys;
		std::vector<int> pressedKeys;
		std::vector<int> releasedKeys;
	};

}