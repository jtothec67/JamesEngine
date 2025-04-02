#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

struct freelookCamController : public Component
{
	float normalSpeed = 5.f;
	float fastSpeed = 30.f;
	float sensitivity = 45.f;

	void OnTick()
	{
		float speed = 0.f;
		if (GetKeyboard()->IsKey(SDLK_LSHIFT))
			speed = fastSpeed;
		else
			speed = normalSpeed;

		if (GetKeyboard()->IsKey(SDLK_w))
			GetTransform()->Move(-GetTransform()->GetForward() * speed * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_s))
			GetTransform()->Move(GetTransform()->GetForward() * speed * GetCore()->DeltaTime());
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
		
	}
};

struct boxController : public Component
{
	std::shared_ptr<Rigidbody> rb;

	void OnTick()
	{
		if (GetKeyboard()->IsKey(SDLK_u))
		{
			rb->ApplyImpulse(glm::vec3(-10, 0, 0) * GetCore()->DeltaTime());
		}

		if (GetKeyboard()->IsKey(SDLK_j))
		{
			rb->ApplyImpulse(glm::vec3(10, 0, 0) * GetCore()->DeltaTime());
		}

		if (GetKeyboard()->IsKey(SDLK_h))
		{
			rb->ApplyImpulse(glm::vec3(0, 0, 10) * GetCore()->DeltaTime());
		}

		if (GetKeyboard()->IsKey(SDLK_k))
		{
			rb->ApplyImpulse(glm::vec3(0, 0, -10) * GetCore()->DeltaTime());
		}
	}

	void OnCollision(std::shared_ptr<Entity> _otherEntity)
	{
		//std::cout << "Collided!" << std::endl;
	}
};

struct CollisionTest : public Component
{
	std::shared_ptr<Rigidbody> rb;

	void OnTick()
	{
		GetTransform()->SetPosition(rb->mCollisionPoint);

		if (GetKeyboard()->IsKeyDown(SDLK_o))
		{
			GetCore()->SetTimeScale(0.1);
		}
		else if (GetKeyboard()->IsKeyDown(SDLK_p))
		{
			GetCore()->SetTimeScale(1);
		}
	}
};

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(ivec2(1920, 1080));
	core->SetTimeScale(1.f);

	// Scope to ensure the entities aren't being held in main if they're destroyed
	{

		core->GetSkybox()->SetTexture(core->GetResources()->Load<SkyboxTexture>("skyboxes/sky"));

		core->GetLightManager()->AddLight("light1", vec3(0, 20, 0), vec3(1, 1, 1), 1.f);
		core->GetLightManager()->SetAmbient(vec3(1.f));

		std::shared_ptr<Entity> cameraEntity = core->AddEntity();
		std::shared_ptr<Camera> camera = cameraEntity->AddComponent<Camera>();
		cameraEntity->GetComponent<Transform>()->SetPosition(vec3(0, 0.f, -0));
		cameraEntity->GetComponent<Transform>()->SetRotation(vec3(0, 90, 0));
		cameraEntity->AddComponent<freelookCamController>();


		std::shared_ptr<Entity> carBody = core->AddEntity();
		carBody->SetTag("carBody");
		carBody->GetComponent<Transform>()->SetPosition(vec3(0, 2.f, -16));
		carBody->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		carBody->GetComponent<Transform>()->SetScale(vec3(1, 0.5, 0.5));
		std::shared_ptr<ModelRenderer> carBodyMR = carBody->AddComponent<ModelRenderer>();
		carBodyMR->SetModel(core->GetResources()->Load<Model>("shapes/cube"));
		carBodyMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<BoxCollider> carBodyCollider = carBody->AddComponent<BoxCollider>();
		carBodyCollider->SetSize(vec3(2, 1, 1));
		std::shared_ptr<Rigidbody> carBodyRB = carBody->AddComponent<Rigidbody>();
		carBodyRB->SetMass(1);



		std::shared_ptr<Entity> wheel = core->AddEntity();
		wheel->SetTag("wheel");
		wheel->GetComponent<Transform>()->SetPosition(vec3(5, 2.f, -16));
		wheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		wheel->GetComponent<Transform>()->SetScale(vec3(0.5, 0.5, 0.15));
		cameraEntity->GetComponent<Transform>()->SetParent(wheel);
		std::shared_ptr<ModelRenderer> wheelMR = wheel->AddComponent<ModelRenderer>();
		wheelMR->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		wheelMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		wheelMR->SetRotationOffset(vec3(90, 0, 0));
		std::shared_ptr<RayCollider> wheelCollider = wheel->AddComponent<RayCollider>();
		wheelCollider->SetDirection(vec3(0, -1, 0));
		wheelCollider->SetLength(0.5);
		std::shared_ptr<Rigidbody> wheelRB = wheel->AddComponent<Rigidbody>();
		wheelRB->SetMass(1);
		wheelRB->LockRotation(true);
		wheel->AddComponent<boxController>()->rb = wheelRB;


		std::shared_ptr<Entity> track = core->AddEntity();
		track->GetComponent<Transform>()->SetPosition(vec3(0, 0, 10));
		track->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		std::shared_ptr<ModelRenderer> trackMR = track->AddComponent<ModelRenderer>();
		trackMR->SetModel(core->GetResources()->Load<Model>("models/track/cartoon_track_trimmed"));
		trackMR->SetTexture(core->GetResources()->Load<Texture>("models/track/rock"));
		std::shared_ptr<ModelCollider> trackCollider = track->AddComponent<ModelCollider>();
		trackCollider->SetModel(core->GetResources()->Load<Model>("models/track/cartoon_track_trimmed"));
		trackCollider->SetRotationOffset(vec3(0, 0, 0));
		trackCollider->SetDebugVisual(false);


		std::shared_ptr<Entity> testEntity = core->AddEntity();
		testEntity->GetComponent<Transform>()->SetPosition(vec3(4.80949, 9.48961, 6.23224));
		testEntity->GetComponent<Transform>()->SetRotation(vec3(0, 90, 0));
		testEntity->GetComponent<Transform>()->SetScale(vec3(0.1, 0.1, 0.1));
		std::shared_ptr<ModelRenderer> testTR = testEntity->AddComponent<ModelRenderer>();
		testTR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		testTR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		testEntity->AddComponent<CollisionTest>()->rb = wheelRB;


	}

	core->Run();
}