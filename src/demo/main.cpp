#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

void InitialiseMenu(std::shared_ptr<Core> core);
void InitialiseGame(std::shared_ptr<Core> core);
void AddCollectable(std::shared_ptr<Core> core, glm::vec3 position);

struct CameraController : public Component
{
	void OnTick()
	{

	}

	void OnDestroy()
	{
		
	}

	void OnGUI()
	{
		
	}

	void OnCollision()
	{
		
	}
};

struct Player : public Component
{
	float speed = 60.f;
	float rotationSpeed = 90.f;
	float downwardSpeed = 0.f;
	float lastY = 0.f;
	bool canJump = true;
	
	int points = 0;

	void OnTick()
	{
		downwardSpeed += 2.f * GetCore()->DeltaTime();

		Move(glm::vec3(0, 1, 0) * -downwardSpeed * GetCore()->DeltaTime());

		if (canJump && GetKeyboard()->IsKeyDown(SDLK_SPACE))
		{
			downwardSpeed = -2;
			//canJump = false;
		}

		if (GetKeyboard()->IsKey(SDLK_w))
		{
			Move(GetTransform()->GetForward() * speed * GetCore()->DeltaTime());
		}

		if (GetKeyboard()->IsKey(SDLK_s))
		{
			Move(-GetTransform()->GetForward() * speed * GetCore()->DeltaTime());
		}

		if (GetKeyboard()->IsKey(SDLK_a))
		{
			//Move(-GetTransform()->GetRight() * speed * GetCore()->DeltaTime());
			Rotate(glm::vec3(0, rotationSpeed * GetCore()->DeltaTime(), 0));
		}

		if (GetKeyboard()->IsKey(SDLK_d))
		{
			//Move(GetTransform()->GetRight() * speed * GetCore()->DeltaTime());
			Rotate(glm::vec3(0, -rotationSpeed * GetCore()->DeltaTime(), 0));
		}

		if (points == 6)
		{
			GetCore()->DestroyAllEntities();
			InitialiseMenu(GetCore());
		}
	}

	void OnCollision(std::string _tag )
	{
		if (_tag == "village")
		{
			canJump = true;
			downwardSpeed = 0;
		}
	}

	void OnGUI()
	{
		int width, height;
		GetCore()->GetWindow()->GetWindowSize(width, height);

		GetGUI()->Text(glm::vec2(width / 2, height - 50), 40, glm::vec3(0, 0, 0), "Mice found: " + std::to_string(points), GetCore()->GetResources()->Load<Font>("fonts/munro"));
	}
};

struct Collectable : public Component
{
	float sinValue = 0;

	float startingY;

	void OnInitialize()
	{
		startingY = GetPosition().y;
	}

	void OnTick()
	{
		Rotate(glm::vec3(0, 90 * GetCore()->DeltaTime(), 0));

		SetPosition(glm::vec3(GetPosition().x, startingY + sin(sinValue) * 0.1, GetPosition().z));

		sinValue += 3 * GetCore()->DeltaTime();
	}

	void OnCollision(std::string _tag)
	{
		if (_tag == "cat")
		{
			GetEntity()->Destroy();

			GetCore()->GetEntityByTag("cat")->GetComponent<Player>()->points++;
		}
	}
};

struct MenuCamera : public Component
{
	void OnTick()
	{
		Rotate(glm::vec3(0, -15 * GetCore()->DeltaTime(), 0));
	}

	void OnGUI()
	{
		int width, height;
		GetCore()->GetWindow()->GetWindowSize(width, height);

		GetGUI()->Text(glm::vec2(width / 2, height - (height / 4)), 40, glm::vec3(0, 0, 0), "Interrupt the talented mice!", GetCore()->GetResources()->Load<Font>("fonts/munro"));

		int start = GetGUI()->Button(glm::vec2(width / 2, height / 2), glm::vec2(200, 100), GetCore()->GetResources()->Load<Texture>("images/BlankButton"));
		if (start == 1)
		{
			GetGUI()->Button(glm::vec2(width / 2, height / 2), glm::vec2(200, 100), GetCore()->GetResources()->Load<Texture>("images/HoveredButton"));
		}
		else if (start == 2)
		{
			GetCore()->DestroyAllEntities();
			InitialiseGame(GetCore());
		}
		GetGUI()->Text(glm::vec2(width / 2, height / 2), 40, glm::vec3(0, 0, 0), "Start", GetCore()->GetResources()->Load<Font>("fonts/munro"));

		int quit = GetGUI()->Button(glm::vec2(width / 2, height / 4), glm::vec2(200, 100), GetCore()->GetResources()->Load<Texture>("images/BlankButton"));
		if (quit == 1)
		{
			GetGUI()->Button(glm::vec2(width / 2, height / 4), glm::vec2(200, 100), GetCore()->GetResources()->Load<Texture>("images/HoveredButton"));
		}
		else if (quit == 2)
		{
			GetCore()->End();
		}
		GetGUI()->Text(glm::vec2(width / 2, height / 4), 40, glm::vec3(0, 0, 0), "Quit", GetCore()->GetResources()->Load<Font>("fonts/munro"));
	}
};


#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(glm::ivec2(800, 600));

	// Scope to ensure the entities aren't being held in main if they're destroyed
	{

		InitialiseMenu(core);

	}

	core->Run();
}

void InitialiseMenu(std::shared_ptr<Core> core)
{
	core->GetSkybox()->SetTexture(core->GetResources()->Load<SkyboxTexture>("skyboxes/sky"));

	std::shared_ptr<Entity> camera = core->AddEntity();
	camera->AddComponent<Camera>();
	camera->AddComponent<MenuCamera>();

}

void InitialiseGame(std::shared_ptr<Core> core)
{
	core->GetSkybox()->SetTexture(core->GetResources()->Load<SkyboxTexture>("skyboxes/sky"));

	std::shared_ptr<Entity> camera = core->AddEntity();
	camera->GetComponent<Transform>()->SetPosition(glm::vec3(0, 1.5, -5));
	std::shared_ptr<Camera> cameraComponent = camera->AddComponent<Camera>();
	camera->AddComponent<CameraController>();

	std::shared_ptr<Entity> city = core->AddEntity();
	city->SetTag("village");
	std::shared_ptr<ModelRenderer> citymr = city->AddComponent<ModelRenderer>();
	citymr->SetModel(core->GetResources()->Load<Model>("models/city/city"));
	citymr->SetTexture(core->GetResources()->Load<Texture>("models/city/lowpoly_tex-2"));
	citymr->SetSpecularStrength(0);
	city->GetComponent<Transform>()->SetScale(glm::vec3(0.01, 0.01, 0.01));
	std::shared_ptr<BoxCollider> citybc = city->AddComponent<BoxCollider>();
	citybc->SetOffset(glm::vec3(21, -0.75, -39)); // Fiddled around to get right numbers
	citybc->SetSize(glm::vec3(120, 1, 130));

	std::shared_ptr<Entity> player = core->AddEntity();
	player->SetTag("cat");
	std::shared_ptr<Transform> playerTransform = player->GetComponent<Transform>();
	playerTransform->SetPosition(glm::vec3(-5, 1, 5));
	playerTransform->SetRotation(glm::vec3(0, 180, 0));
	playerTransform->SetScale(glm::vec3(0.5, 0.5, 0.5));
	std::shared_ptr<ModelRenderer> playermr = player->AddComponent<ModelRenderer>();
	playermr->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
	playermr->SetTexture(core->GetResources()->Load<Texture>("models/curuthers/Whiskers_diffuse"));
	std::shared_ptr<BoxCollider> playerbc = player->AddComponent<BoxCollider>();
	playerbc->SetOffset(glm::vec3(0, 0.15, 0));
	playerbc->SetSize(glm::vec3(0.5, 2.75, 0.5));
	player->AddComponent<Rigidbody>();
	player->AddComponent<Player>();

	camera->GetComponent<Transform>()->SetParent(player);

	AddCollectable(core, glm::vec3(0, 2.5, 0));
	AddCollectable(core, glm::vec3(20, 2.5, -25));
	AddCollectable(core, glm::vec3(65, 2.5, -35));
	AddCollectable(core, glm::vec3(65, 2.5, -70));
	AddCollectable(core, glm::vec3(20, 2.5, -70));
	AddCollectable(core, glm::vec3(-20, 2.5, -70));
}

void AddCollectable(std::shared_ptr<Core> core, glm::vec3 position)
{
	std::shared_ptr<Entity> collectable = core->AddEntity();
	collectable->SetTag("collectable");
	std::shared_ptr<ModelRenderer> collectablemr = collectable->AddComponent<ModelRenderer>();
	collectablemr->SetModel(core->GetResources()->Load<Model>("models/mouse/Homiak_pose2"));
	collectablemr->SetTexture(core->GetResources()->Load<Texture>("models/mouse/lambert1_Base_Color"));
	std::shared_ptr<BoxCollider> collectablebc = collectable->AddComponent<BoxCollider>();
	collectablebc->SetOffset(glm::vec3(0, -0.5, 0));
	collectablebc->SetSize(glm::vec3(1, 1.2, 1));
	collectable->GetComponent<Transform>()->SetPosition(position);
	collectable->GetComponent<Transform>()->SetScale(glm::vec3(0.5, 0.5, 0.5));
	std::shared_ptr<AudioSource> collectableas = collectable->AddComponent<AudioSource>();
	collectableas->SetSound(core->GetResources()->Load<Sound>("sounds/mice band [TubeRipper.cc] mono"));
	collectableas->SetGain(0.05f);
	collectableas->SetLooping(true);
	collectable->AddComponent<Collectable>();
}
