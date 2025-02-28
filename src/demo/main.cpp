#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

struct freelookCamController : public Component
{
	float speed = 100.f;
	float sensitivity = 45.f;

	void OnTick()
	{
		if (GetKeyboard()->IsKey(SDLK_w))
			GetTransform()->Move(GetTransform()->GetForward() * speed * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_s))
			GetTransform()->Move(-GetTransform()->GetForward() * speed * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_a))
			GetTransform()->Move(-GetTransform()->GetRight() * speed * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_d))
			GetTransform()->Move(GetTransform()->GetRight() * speed * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_q))
			GetTransform()->Move(-GetTransform()->GetUp() * speed * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_e))
			GetTransform()->Move(GetTransform()->GetUp() * speed * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_UP))
			GetTransform()->Rotate(vec3(sensitivity * GetCore()->DeltaTime(), 0, 0));
		if (GetKeyboard()->IsKey(SDLK_DOWN))
			GetTransform()->Rotate(vec3(-sensitivity * GetCore()->DeltaTime(), 0, 0));
		if (GetKeyboard()->IsKey(SDLK_LEFT))
			GetTransform()->Rotate(vec3(0, sensitivity * GetCore()->DeltaTime(), 0));
		if (GetKeyboard()->IsKey(SDLK_RIGHT))
			GetTransform()->Rotate(vec3(0, -sensitivity * GetCore()->DeltaTime(), 0));
		if (GetKeyboard()->IsKey(SDLK_LSHIFT))
			speed = 5.f;
		else
			speed = 1.f;
	}
};

struct boxController : public Component
{
	bool moving = false;
	float speed = 1.f;
	float sinValue = 0;
	void OnTick()
	{
		if (GetKeyboard()->IsKeyDown(SDLK_SPACE))
			moving = !moving;

		if (moving)
		{
			GetTransform()->Rotate(glm::vec3(0, 0, 45) * speed * GetCore()->DeltaTime());
			GetTransform()->Move(glm::vec3(0, -1, 0) * speed * GetCore()->DeltaTime());
		}
	}

	void OnCollision(std::shared_ptr<Entity> ent)
	{
		//std::cout << "Collided!" << std::endl;
	}
};

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(ivec2(1920, 1080));

	// Scope to ensure the entities aren't being held in main if they're destroyed
	{

		core->GetSkybox()->SetTexture(core->GetResources()->Load<SkyboxTexture>("skyboxes/sky"));

		core->GetLightManager()->AddLight("light1", vec3(0, 20, 0), vec3(1, 1, 1), 1.f);
		core->GetLightManager()->SetAmbient(vec3(1.f));

		std::shared_ptr<Entity> cameraEntity = core->AddEntity();
		std::shared_ptr<Camera> camera = cameraEntity->AddComponent<Camera>();
		cameraEntity->AddComponent<freelookCamController>();

		std::shared_ptr<Entity> boxEntity = core->AddEntity();
		boxEntity->GetComponent<Transform>()->SetPosition(vec3(0, 20, 10));
		boxEntity->GetComponent<Transform>()->SetRotation(vec3(0, 0, 90));
		boxEntity->GetComponent<Transform>()->SetScale(vec3(0.5, 0.5, 0.5));
		boxEntity->AddComponent<TriangleRenderer>();
		std::shared_ptr<ModelRenderer> boxMR = boxEntity->AddComponent<ModelRenderer>();
		boxMR->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
		boxMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<ModelCollider> boxCollider = boxEntity->AddComponent<ModelCollider>();
		boxCollider->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
		boxEntity->AddComponent<boxController>();

		std::shared_ptr<Entity> boxEntity2 = core->AddEntity();
		boxEntity2->GetComponent<Transform>()->SetPosition(vec3(5, 20, 10));
		boxEntity2->GetComponent<Transform>()->SetRotation(vec3(0, 0, 90));
		boxEntity2->GetComponent<Transform>()->SetScale(vec3(0.5, 0.5, 0.5));
		boxEntity2->AddComponent<TriangleRenderer>();
		std::shared_ptr<ModelRenderer> boxMR2 = boxEntity2->AddComponent<ModelRenderer>();
		boxMR2->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		boxMR2->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<ModelCollider> boxCollider2 = boxEntity2->AddComponent<ModelCollider>();
		boxCollider2->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		boxEntity2->AddComponent<boxController>();

		std::shared_ptr<Entity> boxEntity3 = core->AddEntity();
		boxEntity3->GetComponent<Transform>()->SetPosition(vec3(10, 20, 10));
		boxEntity3->GetComponent<Transform>()->SetRotation(vec3(0, 0, 90));
		boxEntity3->GetComponent<Transform>()->SetScale(vec3(0.5, 0.5, 0.5));
		boxEntity3->AddComponent<TriangleRenderer>();
		std::shared_ptr<ModelRenderer> boxMR3 = boxEntity3->AddComponent<ModelRenderer>();
		boxMR3->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		boxMR3->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<ModelCollider> boxCollider3 = boxEntity3->AddComponent<ModelCollider>();
		boxCollider3->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		boxEntity3->AddComponent<boxController>();

		std::shared_ptr<Entity> boxEntity4 = core->AddEntity();
		boxEntity4->GetComponent<Transform>()->SetPosition(vec3(15, 20, 10));
		boxEntity4->GetComponent<Transform>()->SetRotation(vec3(0, 0, 90));
		boxEntity4->GetComponent<Transform>()->SetScale(vec3(0.5, 0.5, 0.5));
		boxEntity4->AddComponent<TriangleRenderer>();
		std::shared_ptr<ModelRenderer> boxMR4 = boxEntity4->AddComponent<ModelRenderer>();
		boxMR4->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		boxMR4->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<ModelCollider> boxCollider4 = boxEntity4->AddComponent<ModelCollider>();
		boxCollider4->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		boxEntity4->AddComponent<boxController>();


		std::shared_ptr<Entity> mouseEntity = core->AddEntity();
		mouseEntity->GetComponent<Transform>()->SetPosition(vec3(0, 0, 10));
		mouseEntity->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		//mouseEntity->AddComponent<TriangleRenderer>();
		std::shared_ptr<ModelRenderer> mouseMR = mouseEntity->AddComponent<ModelRenderer>();
		mouseMR->SetModel(core->GetResources()->Load<Model>("models/track/cartoon_track"));
		mouseMR->SetTexture(core->GetResources()->Load<Texture>("models/track/rock"));
		std::shared_ptr<ModelCollider> mouseCollider = mouseEntity->AddComponent<ModelCollider>();
		mouseCollider->SetModel(core->GetResources()->Load<Model>("models/track/cartoon_track"));
		mouseCollider->SetRotationOffset(vec3(0, 0, 0));
		/*std::shared_ptr<CapsuleCollider> mouseCollider = mouseEntity->AddComponent<CapsuleCollider>();
		mouseCollider->SetRotationOffset(vec3(0, 0, 0));*/

		boxEntity->AddComponent<Rigidbody>();
		boxEntity2->AddComponent<Rigidbody>();
		boxEntity3->AddComponent<Rigidbody>();
		boxEntity4->AddComponent<Rigidbody>();

	}

	core->Run();
}