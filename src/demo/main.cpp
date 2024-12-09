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
		if (GetInput()->GetKeyboard()->IsKey(SDLK_a))
		{
			GetTransform()->SetPosition(GetTransform()->GetPosition() + glm::vec3(-0.1f, 0.f, 0.f));
		}

		if (GetInput()->GetMouse()->IsButtonDown(SDL_BUTTON_LEFT))
		{
			GetEntity()->GetComponent<AudioSource>()->Play();
		}

		if (GetInput()->GetMouse()->IsButtonDown(SDL_BUTTON_RIGHT))
		{
			std::cout << "Mouse y: " << GetInput()->GetMouse()->GetYPosition() << std::endl;
		}

		if (GetInput()->GetKeyboard()->IsKey(SDLK_d))
		{
			GetTransform()->SetPosition(GetTransform()->GetPosition() + glm::vec3(0.1f, 0.f, 0.f));
		}

		if (GetInput()->GetKeyboard()->IsKey(SDLK_w))
		{
			GetTransform()->SetPosition(GetTransform()->GetPosition() + glm::vec3(0.f, 0.1f, 0.f));
		}

		if (GetInput()->GetKeyboard()->IsKey(SDLK_s))
		{
			GetTransform()->SetPosition(GetTransform()->GetPosition() + glm::vec3(0.f, -0.1f, 0.f));
		}
	}
};

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(glm::ivec2(800,600));

	std::shared_ptr<Entity> ent = core->AddEntity();

	ent->GetComponent<Transform>()->SetPosition(glm::vec3(0.0f, 0.0f, -10.f));
	ent->GetComponent<Transform>()->SetRotation(glm::vec3(0.0f, 90.0f, 0.f));

	ent->AddComponent<Player>();
	std::shared_ptr<ModelRenderer> mr = ent->AddComponent<ModelRenderer>();

	mr->SetModel(core->GetResources()->Load<Model>("models/car/formula_car"));
	mr->SetTexture(core->GetResources()->Load<Texture>("models/car/car_02_m"));

	std::shared_ptr<AudioSource> as = ent->AddComponent<AudioSource>();

	as->SetSound(core->GetResources()->Load<Sound>("sounds/dixie_horn"));

	core->Run();
}