#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

struct Player : public Component
{
	void OnInitialize()
	{
		
	}

	void OnTick()
	{
		float deltaTime = GetEntity()->GetCore()->DeltaTime();
		float speed = 40.f;

		if (GetMouse()->IsButtonDown(SDL_BUTTON_LEFT))
		{
			GetEntity()->GetComponent<AudioSource>()->Play();
		}

		if (GetMouse()->IsButtonDown(SDL_BUTTON_RIGHT))
		{
			std::cout << "Mouse y: " << GetMouse()->GetYPosition() << std::endl;
		}

		if (GetKeyboard()->IsKey(SDLK_RIGHT	))
		{
			Move(glm::vec3(-speed * deltaTime, 0.f, 0.f));
		}

		if (GetKeyboard()->IsKey(SDLK_LEFT))
		{
			Move(glm::vec3(speed * deltaTime, 0.f, 0.f));
		}

		if (GetKeyboard()->IsKey(SDLK_UP))
		{
			Move(glm::vec3(0.f, speed * deltaTime, 0.f));
		}

		if (GetKeyboard()->IsKey(SDLK_DOWN))
		{
			Move(glm::vec3(0.f, -speed * deltaTime, 0.f));
		}
	}

	void OnGUI()
	{
		int width, height;
		GetEntity()->GetCore()->GetWindow()->GetWindowSize(width, height);

		GetGUI()->Image(glm::vec2(0, height - 100), glm::vec2(100, 100), GetEntity()->GetCore()->GetResources()->Load<Texture>("models/car/car_02_m"));

		int buttonAction = GetGUI()->Button(glm::vec2(0, 0), glm::vec2(100, 100), GetEntity()->GetCore()->GetResources()->Load<Texture>("models/car/car_02_m"));
		if (buttonAction == 1)
		{
			//std::cout << "Mouse over button" << std::endl;
			GetGUI()->Button(glm::vec2(0, 0), glm::vec2(100, 100), GetEntity()->GetCore()->GetResources()->Load<Texture>("curuthers/Whiskers_diffuse"));
		}
		else if (buttonAction == 2)
		{
			std::cout << "Button clicked" << std::endl;
		}

		GetGUI()->Text(glm::vec2(0, height / 2), 0.5, glm::vec3(0.f, 0.f, 0.f), "Hello World", GetEntity()->GetCore()->GetResources()->Load<Font>("fonts/munro"));
	}
};

struct CameraController : public Component
{
	void OnTick()
	{
		float deltaTime = GetEntity()->GetCore()->DeltaTime();
		float speed = 40.f;

		if (GetKeyboard()->IsKey(SDLK_a))
		{
			Move(-GetTransform()->GetRight() * speed * deltaTime);
		}

		if (GetKeyboard()->IsKey(SDLK_d))
		{
			Move(GetTransform()->GetRight() * speed * deltaTime);
		}

		if (GetKeyboard()->IsKey(SDLK_w))
		{
			Move(GetTransform()->GetForward() * speed * deltaTime);
		}

		if (GetKeyboard()->IsKey(SDLK_s))
		{
			Move(-GetTransform()->GetForward() * speed * deltaTime);
		}

		if (GetKeyboard()->IsKey(SDLK_q))
		{
			Rotate(glm::vec3(0.f, speed * deltaTime, 0.f));
		}

		if (GetKeyboard()->IsKey(SDLK_e))
		{
			Rotate(glm::vec3(0.f, -speed * deltaTime, 0.f));
		}

		if (GetKeyboard()->IsKey(SDLK_p))
		{
			GetEntity()->GetComponent<Camera>()->SetPriority(-1);
		}

		if (GetKeyboard()->IsKey(SDLK_o))
		{
			GetEntity()->GetComponent<Camera>()->SetPriority(2);
		}
	}
};


#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(glm::ivec2(800,600));

	std::shared_ptr<Entity> camera = core->AddEntity();
	camera->AddComponent<Camera>();
	camera->AddComponent<CameraController>();

	std::shared_ptr<Entity> camera2 = core->AddEntity();
	camera2->AddComponent<Camera>();
	camera2->GetComponent<Transform>()->SetPosition(glm::vec3(0.f, 0.f, 20.f));
	camera2->GetComponent<Transform>()->SetRotation(glm::vec3(0.f, 180.f, 0.f));

	std::shared_ptr<Entity> player = core->AddEntity();
	player->GetComponent<Transform>()->SetPosition(glm::vec3(-5.f, 0.f, 10.f));
	player->GetComponent<Transform>()->SetRotation(glm::vec3(0.f, 180.f, 0.f));
	player->AddComponent<Player>();
	std::shared_ptr<ModelRenderer> entityModelRenderer = player->AddComponent<ModelRenderer>();
	entityModelRenderer->SetModel(core->GetResources()->Load<Model>("models/car/formula_car"));
	entityModelRenderer->SetTexture(core->GetResources()->Load<Texture>("models/car/car_02_m"));
	std::shared_ptr<AudioSource> entityAudioSource = player->AddComponent<AudioSource>();
	entityAudioSource->SetSound(core->GetResources()->Load<Sound>("sounds/dixie_horn_mono"));
	std::shared_ptr<BoxCollider> entityBoxCollider = player->AddComponent<BoxCollider>();
	entityBoxCollider->SetOffset(glm::vec3(0, 0.2f, 0.54f));
	entityBoxCollider->SetSize(glm::vec3(3.2f, 1.7f, 8.4f));

	std::shared_ptr<Entity> cat = core->AddEntity();
	cat->GetComponent<Transform>()->SetPosition(glm::vec3(0.f, 0.f, 10.f));
	cat->GetComponent<Transform>()->SetRotation(glm::vec3(0.f, 90.f, 0.f));
	cat->AddComponent<Rigidbody>();
	std::shared_ptr<ModelRenderer> entityModelRenderer2 = cat->AddComponent<ModelRenderer>();
	entityModelRenderer2->SetModel(core->GetResources()->Load<Model>("curuthers/curuthers"));
	entityModelRenderer2->SetTexture(core->GetResources()->Load<Texture>("curuthers/Whiskers_diffuse"));
	std::shared_ptr<BoxCollider> entityBoxCollider2 = cat->AddComponent<BoxCollider>();

	core->Run();
}