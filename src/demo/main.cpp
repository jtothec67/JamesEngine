#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(ivec2(800, 600));

	// Scope to ensure the entities aren't being held in main if they're destroyed
	{

		

	}

	core->Run();
}