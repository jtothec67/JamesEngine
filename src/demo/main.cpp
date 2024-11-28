#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

class Player : public Component
{
public:
	void OnInitialize()
	{
		
	}

	void OnTick()
	{
		if (GetEntity()->GetCore()->GetInput()->GetKeyboard()->IsKey(SDLK_a))
		{
			GetEntity()->GetComponent<Transform>()->SetPosition(GetEntity()->GetComponent<Transform>()->GetPosition() + glm::vec3(-0.1f, 0.f, 0.f));
		}

		if (GetEntity()->GetCore()->GetInput()->GetKeyboard()->IsKey(SDLK_d))
		{
			GetEntity()->GetComponent<Transform>()->SetPosition(GetEntity()->GetComponent<Transform>()->GetPosition() + glm::vec3(0.1f, 0.f, 0.f));
		}
	}
};

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize();

	std::shared_ptr<Entity> ent = core->AddEntity();

	ent->GetComponent<Transform>()->SetPosition(glm::vec3(0.0f, 0.0f, -10.f));
	ent->GetComponent<Transform>()->SetRotation(glm::vec3(0.0f, 90.0f, 0.f));

	ent->AddComponent<Player>();
	std::shared_ptr<ModelRenderer> mr = ent->AddComponent<ModelRenderer>();

	mr->SetModel(core->GetResources()->Load<Model>("models/car/formula_car"));
	mr->SetTexture(core->GetResources()->Load<Texture>("models/car/car_02_m"));

	core->Run();
}