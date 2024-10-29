#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

class Player : public Component
{
public:
	void OnInitialize()
	{
		std::cout << "Player initialize" << std::endl;
	}

	void OnTick()
	{
		//std::cout << "Player tick" << std::endl;
	}
};

int main()
{
	std::shared_ptr<Core> core = Core::Initialize();

	std::shared_ptr<Entity> ent = core->AddEntity();

	ent->AddComponent<Player>();

	core->Run();
}