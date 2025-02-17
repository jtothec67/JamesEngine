#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

struct freelookCamController : public Component
{
	float speed = 1.f;
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
	void OnTick()
	{
		if (GetKeyboard()->IsKeyDown(SDLK_SPACE))
			moving = !moving;

		if (moving)
		{
			//GetTransform()->Rotate(glm::vec3(0, 45, 0) * speed * GetCore()->DeltaTime());
			GetTransform()->Move(glm::vec3(0, -1, 0) * -speed * GetCore()->DeltaTime());
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
		boxEntity->GetComponent<Transform>()->SetPosition(vec3(-4, 0, 10));
		boxEntity->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		std::shared_ptr<ModelRenderer> boxMR = boxEntity->AddComponent<ModelRenderer>();
		boxMR->SetModel(core->GetResources()->Load<Model>("shapes/capsule"));
		boxMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		/*std::shared_ptr<CapsuleCollider> boxCollider = boxEntity->AddComponent<CapsuleCollider>();
		boxCollider->SetHeight(1);
		boxCollider->SetCylinderRadius(1);
		boxCollider->SetCapRadius(3);*/
		std::shared_ptr<ModelCollider> boxCollider = boxEntity->AddComponent<ModelCollider>();
		boxCollider->SetModel(core->GetResources()->Load<Model>("shapes/capsule"));
		boxCollider->SetRotationOffset(vec3(0, 0, 0));
		boxEntity->AddComponent<Rigidbody>();

		boxEntity->AddComponent<boxController>();
		

		std::shared_ptr<Entity> mouseEntity = core->AddEntity();
		mouseEntity->GetComponent<Transform>()->SetPosition(vec3(0, 0, 10));
		mouseEntity->GetComponent<Transform>()->SetRotation(vec3(0, 180, -0));
		std::shared_ptr<ModelRenderer> mouseMR = mouseEntity->AddComponent<ModelRenderer>();
		mouseMR->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
		mouseMR->SetTexture(core->GetResources()->Load<Texture>("models/curuthers/Whiskers_diffuse"));
		mouseEntity->AddComponent<TriangleRenderer>();
		std::shared_ptr<ModelCollider> mouseCollider = mouseEntity->AddComponent<ModelCollider>();
		mouseCollider->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
		mouseCollider->SetRotationOffset(vec3(0, 0, 0));
		mouseEntity->AddComponent<Rigidbody>();
		mouseEntity->AddComponent<boxController>();
		/*std::shared_ptr<CapsuleCollider> mouseCollider = mouseEntity->AddComponent<CapsuleCollider>();
		mouseCollider->SetRotationOffset(vec3(0, 0, 0));*/


		std::shared_ptr<Entity> entity1 = core->AddEntity();
		entity1->GetComponent<Transform>()->SetPosition(vec3(2, 0, 5));
		entity1->GetComponent<Transform>()->SetRotation(vec3(0, 45, 0));
		std::shared_ptr<ModelRenderer> entity1MR = entity1->AddComponent<ModelRenderer>();
		entity1MR->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		entity1MR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<ModelCollider> entity1Collider = entity1->AddComponent<ModelCollider>();
		entity1Collider->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
		entity1Collider->SetRotationOffset(vec3(0, 0, 0));
		entity1->AddComponent<Rigidbody>();
		entity1->AddComponent<boxController>();

		std::shared_ptr<Entity> entity2 = core->AddEntity();
		entity2->GetComponent<Transform>()->SetPosition(vec3(-2, 0, 5));
		entity2->GetComponent<Transform>()->SetRotation(vec3(0, -45, 0));
		std::shared_ptr<ModelRenderer> entity2MR = entity2->AddComponent<ModelRenderer>();
		entity2MR->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		entity2MR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<ModelCollider> entity2Collider = entity2->AddComponent<ModelCollider>();
		entity2Collider->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
		entity2Collider->SetRotationOffset(vec3(0, 0, 0));
		entity2->AddComponent<Rigidbody>();
		entity2->AddComponent<boxController>();

		std::shared_ptr<Entity> floorEntity = core->AddEntity();
		floorEntity->GetComponent<Transform>()->SetPosition(vec3(0, 10, 10));
		floorEntity->GetComponent<Transform>()->SetScale(vec3(10, 1, 10));
		std::shared_ptr<ModelRenderer> floorMR = floorEntity->AddComponent<ModelRenderer>();
		floorMR->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
		floorMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<ModelCollider> floorCollider = floorEntity->AddComponent<ModelCollider>();
		floorCollider->SetModel(core->GetResources()->Load<Model>("models/curuthers/curuthers"));
		floorCollider->SetRotationOffset(vec3(0, 0, 0));

	}

	core->Run();
}