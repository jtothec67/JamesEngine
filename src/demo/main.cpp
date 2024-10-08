#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

class Test : public Component
{
public:
	int mTemp;
};

int main()
{
	std::shared_ptr<Core> core = Core::Initialize();

	std::shared_ptr<Entity> ent = core->AddEntity();
	ent->AddComponent<Test>();

	core->Start();
}