#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

class Test : Component
{
	int mTemp;
};



int main()
{
	std::shared_ptr<JamesEngine::Core> core = JamesEngine::Core::Initialize();

	std::shared_ptr<Entity> ent = core->AddEntity();
	ent->AddComponent<Test>();

	core->Start();
}