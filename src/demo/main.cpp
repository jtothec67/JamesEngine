#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

struct CameraController : public Component
{
	float speed = 3.f;
	float rotationSpeed = 90.f;

	void OnTick()
	{
		//if (GetKeyboard()->IsKey(SDLK_w))
		//{
		//	Move(GetTransform()->GetForward() * speed * GetCore()->DeltaTime());
		//}

		//if (GetKeyboard()->IsKey(SDLK_s))
		//{
		//	Move(-GetTransform()->GetForward() * speed * GetCore()->DeltaTime());
		//}

		//if (GetKeyboard()->IsKey(SDLK_a))
		//{
		//	//Move(-GetTransform()->GetRight() * speed * GetCore()->DeltaTime());
		//	Rotate(glm::vec3(0, rotationSpeed * GetCore()->DeltaTime(), 0));
		//}

		//if (GetKeyboard()->IsKey(SDLK_d))
		//{
		//	//Move(GetTransform()->GetRight() * speed * GetCore()->DeltaTime());
		//	Rotate(glm::vec3(0, -rotationSpeed * GetCore()->DeltaTime(), 0));
		//}

		if (GetKeyboard()->IsKey(SDLK_SPACE))
		{
			Move(glm::vec3(0, speed * GetCore()->DeltaTime(), 0));
		}

		if (GetKeyboard()->IsKey(SDLK_LSHIFT))
		{
			Move(glm::vec3(0, -speed * GetCore()->DeltaTime(), 0));
		}
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

struct Object : public Component
{
	float speed = 3.f;
	float rotationSpeed = 90.f;

	void OnTick()
	{
		Move(glm::vec3(0, 1, 0) * -1.f * GetCore()->DeltaTime());

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
	}
};


#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(glm::ivec2(800, 600));

	// Scope to ensure the entities aren't being held in main if they're destroyed
	{

		core->GetSkybox()->SetTexture(core->GetResources()->Load<SkyboxTexture>("skyboxes/sky"));

		std::shared_ptr<Entity> camera = core->AddEntity();
		camera->GetComponent<Transform>()->SetPosition(glm::vec3(0, 1.5, -5));
		std::shared_ptr<Camera> cameraComponent = camera->AddComponent<Camera>();
		camera->AddComponent<CameraController>();

		std::shared_ptr<Entity> village = core->AddEntity();
		std::shared_ptr<ModelRenderer> villagemr = village->AddComponent<ModelRenderer>();
		villagemr->SetModel(core->GetResources()->Load<Model>("models/village/pme41111"));
		villagemr->SetTexture(core->GetResources()->Load<Texture>("models/village/pme41111-RGBA"));
		village->GetComponent<Transform>()->SetPosition(glm::vec3(0, -35, 0));
		std::shared_ptr<BoxCollider> villagebc = village->AddComponent<BoxCollider>();
		villagebc->SetOffset(glm::vec3(0, 34, 0));
		villagebc->SetSize(glm::vec3(50, 1, 50));

		std::shared_ptr<Entity> object = core->AddEntity();
		object->GetComponent<Transform>()->SetPosition(glm::vec3(0, 1, 0));
		object->GetComponent<Transform>()->SetScale(glm::vec3(0.5, 0.5, 0.5));
		std::shared_ptr<ModelRenderer> objectmr = object->AddComponent<ModelRenderer>();
		objectmr->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
		objectmr->SetTexture(core->GetResources()->Load<Texture>("models/curuthers/Whiskers_diffuse"));
		std::shared_ptr<BoxCollider> objectbc = object->AddComponent<BoxCollider>();
		objectbc->SetOffset(glm::vec3(0, 0.15, 0));
		objectbc->SetSize(glm::vec3(0.5, 2.75, 0.5));
		object->AddComponent<Rigidbody>();
		object->AddComponent<Object>();

		camera->GetComponent<Transform>()->SetParent(object);

	}

	core->Run();
}