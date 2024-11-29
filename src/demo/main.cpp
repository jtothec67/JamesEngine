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

		if (GetInput()->GetMouse()->IsButton(SDL_BUTTON_LEFT))
		{
			std::cout << "Mouse x: " << GetInput()->GetMouse()->GetXPosition() << std::endl;
		}

		if (GetInput()->GetMouse()->IsButton(SDL_BUTTON_RIGHT))
		{
			GetTransform()->SetPosition(GetTransform()->GetPosition() + glm::vec3(0.1f, 0.f, 0.f));
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

		std::cout << "Mouse delta: " << GetInput()->GetMouse()->GetDelta().x << ", " << GetInput()->GetMouse()->GetDelta().y << std::endl;
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