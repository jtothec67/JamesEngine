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
		float deltaTime = GetEntity()->GetCore()->DeltaTime();
		float speed = 4.f;

		if (GetKeyboard()->IsKey(SDLK_a))
		{
			SetPosition(GetPosition() + glm::vec3(-speed * deltaTime, 0.f, 0.f));
		}

		if (GetMouse()->IsButtonDown(SDL_BUTTON_LEFT))
		{
			GetEntity()->GetComponent<AudioSource>()->Play();
		}

		if (GetMouse()->IsButtonDown(SDL_BUTTON_RIGHT))
		{
			std::cout << "Mouse y: " << GetMouse()->GetYPosition() << std::endl;
		}

		if (GetKeyboard()->IsKey(SDLK_d))
		{
			SetPosition(GetPosition() + glm::vec3(speed * deltaTime, 0.f, 0.f));
		}

		if (GetKeyboard()->IsKey(SDLK_w))
		{
			SetPosition(GetPosition() + glm::vec3(0.f, speed * deltaTime, 0.f));
		}

		if (GetKeyboard()->IsKey(SDLK_s))
		{
			SetPosition(GetPosition() + glm::vec3(0.f, -speed * deltaTime, 0.f));
		}
	}
};

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(glm::ivec2(800,600));

	std::shared_ptr<Entity> player = core->AddEntity();

	player->GetComponent<Transform>()->SetPosition(glm::vec3(-5.0f, 0.0f, -10.f));
	player->GetComponent<Transform>()->SetRotation(glm::vec3(0.0f, 0.0f, 0.f));

	player->AddComponent<Player>();
	std::shared_ptr<ModelRenderer> entityModelRenderer = player->AddComponent<ModelRenderer>();

	entityModelRenderer->SetModel(core->GetResources()->Load<Model>("models/car/formula_car"));
	entityModelRenderer->SetTexture(core->GetResources()->Load<Texture>("models/car/car_02_m"));

	std::shared_ptr<AudioSource> entityAudioSource = player->AddComponent<AudioSource>();

	entityAudioSource->SetSound(core->GetResources()->Load<Sound>("sounds/dixie_horn"));

	std::shared_ptr<BoxCollider> entityBoxCollider = player->AddComponent<BoxCollider>();
	entityBoxCollider->SetOffset(glm::vec3(0, 0.2f, 0.54f));
	entityBoxCollider->SetSize(glm::vec3(3.2f, 1.7f, 8.4f));

	std::shared_ptr<Entity> cat = core->AddEntity();
	cat->GetComponent<Transform>()->SetPosition(glm::vec3(0.0f, 0.0f, -10.f));
	cat->GetComponent<Transform>()->SetRotation(glm::vec3(0.0f, 90.0f, 0.f));

	cat->AddComponent<Rigidbody>();

	std::shared_ptr<ModelRenderer> entityModelRenderer2 = cat->AddComponent<ModelRenderer>();

	entityModelRenderer2->SetModel(core->GetResources()->Load<Model>("curuthers/curuthers"));
	entityModelRenderer2->SetTexture(core->GetResources()->Load<Texture>("curuthers/Whiskers_diffuse"));

	std::shared_ptr<BoxCollider> entityBoxCollider2 = cat->AddComponent<BoxCollider>();

	core->Run();
}