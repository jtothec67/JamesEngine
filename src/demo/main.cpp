#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

class Player : public Component
{
public:
	void OnInitialize()
	{
		//std::cout << "Player initialize" << std::endl;
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

	ent->GetComponent<Transform>()->SetPosition(glm::vec3(0.0f, 0.0f, -10.f));

	ent->AddComponent<Player>();
	std::shared_ptr<ModelRenderer> mr = ent->AddComponent<ModelRenderer>();

	mr->SetModel(core->GetResources()->Load<Model>("models/car/formula_car"));
	mr->SetTexture(core->GetResources()->Load<Texture>("models/car/car_02_m"));

	core->Run();
}