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

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize();

	std::shared_ptr<Entity> ent = core->AddEntity();

	ent->GetComponent<Transform>()->position.z = -3;

	ent->AddComponent<Player>();
	//ent->AddComponent<ModelRenderer("", "")>(); // How

	core->Run();
}