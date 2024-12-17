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
		if (GetKeyboard()->IsKey(SDLK_a))
		{
			SetPosition(GetPosition() + glm::vec3(-0.1f, 0.f, 0.f));
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
			SetPosition(GetPosition() + glm::vec3(0.1f, 0.f, 0.f));
		}

		if (GetKeyboard()->IsKey(SDLK_w))
		{
			SetPosition(GetPosition() + glm::vec3(0.f, 0.1f, 0.f));
		}

		if (GetKeyboard()->IsKey(SDLK_s))
		{
			SetPosition(GetPosition() + glm::vec3(0.f, -0.1f, 0.f));
		}
	}
};

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(glm::ivec2(800,600));

	std::shared_ptr<Entity> entity = core->AddEntity();

	entity->GetComponent<Transform>()->SetPosition(glm::vec3(-5.0f, 0.0f, -10.f));
	entity->GetComponent<Transform>()->SetRotation(glm::vec3(0.0f, 90.0f, 0.f));

	entity->AddComponent<Player>();
	std::shared_ptr<ModelRenderer> entityModelRenderer = entity->AddComponent<ModelRenderer>();

	entityModelRenderer->SetModel(core->GetResources()->Load<Model>("models/car/formula_car"));
	entityModelRenderer->SetTexture(core->GetResources()->Load<Texture>("models/car/car_02_m"));

	std::shared_ptr<AudioSource> entityAudioSource = entity->AddComponent<AudioSource>();

	entityAudioSource->SetSound(core->GetResources()->Load<Sound>("sounds/dixie_horn"));

	std::shared_ptr<BoxCollider> entityBoxCollider = entity->AddComponent<BoxCollider>();
	entityBoxCollider->SetOffset(glm::vec3(0, 0.2f, 0.54f));
	entityBoxCollider->SetSize(glm::vec3(3.2f, 1.7f, 8.4f));
	entity->AddComponent<Rigidbody>();

	std::shared_ptr<Entity> entity2 = core->AddEntity();
	entity2->GetComponent<Transform>()->SetPosition(glm::vec3(0.0f, 0.0f, -10.f));
	entity2->GetComponent<Transform>()->SetRotation(glm::vec3(0.0f, 90.0f, 0.f));

	std::shared_ptr<ModelRenderer> entityModelRenderer2 = entity2->AddComponent<ModelRenderer>();

	entityModelRenderer2->SetModel(core->GetResources()->Load<Model>("curuthers/curuthers"));
	entityModelRenderer2->SetTexture(core->GetResources()->Load<Texture>("curuthers/Whiskers_diffuse"));

	std::shared_ptr<BoxCollider> entityBoxCollider2 = entity2->AddComponent<BoxCollider>();

	core->Run();
}