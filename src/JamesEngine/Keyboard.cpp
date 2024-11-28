#include "Keyboard.h"

#include <iostream>

namespace JamesEngine
{

	void Keyboard::Update()
	{
		pressedKeys.clear();
		releasedKeys.clear();
	}

	bool Keyboard::IsKey(int _key)
	{
		for (int i = 0; i < keys.size(); ++i)
		{
			
			if (keys[i] == _key)
			{
				return true;
			}
		}
		return false;
	}

	bool Keyboard::IsKeyDown(int _key)
	{
		for (int i = 0; i < pressedKeys.size(); ++i)
		{
			if (pressedKeys[i] == _key)
			{
				return true;
			}
		}
		return false;
	}

	bool Keyboard::IsKeyUp(int _key)
	{
		for (int i = 0; i < releasedKeys.size(); ++i)
		{
			if (releasedKeys[i] == _key)
			{
				return true;
			}
		}
		return false;
	}

}