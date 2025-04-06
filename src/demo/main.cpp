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
	std::shared_ptr<Entity> carBody;

	float speed = 100.f;

	void OnTick()
	{
		/*if (GetKeyboard()->IsKey(SDLK_u))
		{
			rb->ApplyImpulse(carBody->GetComponent<Transform>()->GetRight() * speed * GetCore()->DeltaTime());
			std::cout << "u" << std::endl;
		}

		if (GetKeyboard()->IsKey(SDLK_j))
		{
			rb->ApplyImpulse(-carBody->GetComponent<Transform>()->GetRight() * speed * GetCore()->DeltaTime());
			std::cout << "j" << std::endl;
		}*/

		/*if (GetKeyboard()->IsKey(SDLK_h))
		{
			rb->ApplyImpulse(-carBody->GetComponent<Transform>()->GetForward() * speed * GetCore()->DeltaTime());
		}

		if (GetKeyboard()->IsKey(SDLK_k))
		{
			rb->ApplyImpulse(carBody->GetComponent<Transform>()->GetForward() * speed * GetCore()->DeltaTime());
		}*/
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

struct CarController : public Component
{
	std::shared_ptr<Rigidbody> rb;

	float forwardSpeed = 50.f;
	float turnSpeed = 10000.f;

	void OnTick()
	{
		if (GetKeyboard()->IsKey(SDLK_h))
		{
			rb->AddTorque(GetEntity()->GetComponent<Transform>()->GetUp() * turnSpeed * GetCore()->DeltaTime());
			std::cout << "h" << std::endl;
		}

		if (GetKeyboard()->IsKey(SDLK_k))
		{
			rb->AddTorque(-GetEntity()->GetComponent<Transform>()->GetUp() * turnSpeed * GetCore()->DeltaTime());
			std::cout << "k" << std::endl;
		}

		if (GetKeyboard()->IsKey(SDLK_u))
		{
			rb->ApplyImpulse(GetEntity()->GetComponent<Transform>()->GetRight() * forwardSpeed * GetCore()->DeltaTime());
			std::cout << "u" << std::endl;
		}

		if (GetKeyboard()->IsKey(SDLK_j))
		{
			rb->ApplyImpulse(-GetEntity()->GetComponent<Transform>()->GetRight() * forwardSpeed * GetCore()->DeltaTime());
			std::cout << "j" << std::endl;
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

		// Camera
		std::shared_ptr<Entity> cameraEntity = core->AddEntity();
		std::shared_ptr<Camera> camera = cameraEntity->AddComponent<Camera>();
		cameraEntity->GetComponent<Transform>()->SetPosition(vec3(-0.5, 1, 0));
		cameraEntity->GetComponent<Transform>()->SetRotation(vec3(0, -90, 0));
		cameraEntity->AddComponent<freelookCamController>();

		// Car Body
		std::shared_ptr<Entity> carBody = core->AddEntity();
		carBody->SetTag("carBody");
		carBody->GetComponent<Transform>()->SetPosition(vec3(0, 0.75, -16));
		carBody->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		carBody->GetComponent<Transform>()->SetScale(vec3(1, 0.25, 0.5));
		std::shared_ptr<ModelRenderer> carBodyMR = carBody->AddComponent<ModelRenderer>();
		carBodyMR->SetModel(core->GetResources()->Load<Model>("shapes/cube"));
		carBodyMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<BoxCollider> carBodyCollider = carBody->AddComponent<BoxCollider>();
		carBodyCollider->SetSize(vec3(2, 0.5, 0.5));
		std::shared_ptr<Rigidbody> carBodyRB = carBody->AddComponent<Rigidbody>();
		carBodyRB->SetMass(1);
		carBody->AddComponent<CarController>()->rb = carBodyRB;
		cameraEntity->GetComponent<Transform>()->SetParent(carBody);

		// Front Left Wheel
		std::shared_ptr<Entity> FLWheel = core->AddEntity();
		FLWheel->SetTag("wheel");
		FLWheel->GetComponent<Transform>()->SetPosition(vec3(-1, 0.25, -15.5));
		FLWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		FLWheel->GetComponent<Transform>()->SetScale(vec3(0.5, 0.5, 0.15));
		std::shared_ptr<ModelRenderer> FLWheelMR = FLWheel->AddComponent<ModelRenderer>();
		FLWheelMR->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		FLWheelMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		FLWheelMR->SetRotationOffset(vec3(90, 0, 0));
		std::shared_ptr<RayCollider> FLWheelCollider = FLWheel->AddComponent<RayCollider>();
		FLWheelCollider->SetDirection(vec3(0, -1, 0));
		FLWheelCollider->SetLength(0.5);
		std::shared_ptr<Rigidbody> FLWheelRB = FLWheel->AddComponent<Rigidbody>();
		FLWheelRB->SetMass(5);
		FLWheelRB->LockRotation(true);
		std::shared_ptr<boxController> FLBC = FLWheel->AddComponent<boxController>();
		FLBC->rb = FLWheelRB;
		FLBC->carBody = carBody;

		// Front Right Wheel
		std::shared_ptr<Entity> FRWheel = core->AddEntity();
		FRWheel->SetTag("wheel");
		FRWheel->GetComponent<Transform>()->SetPosition(vec3(1, 0.25, -15.5));
		FRWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		FRWheel->GetComponent<Transform>()->SetScale(vec3(0.5, 0.5, 0.15));
		std::shared_ptr<ModelRenderer> FRWheelMR = FRWheel->AddComponent<ModelRenderer>();
		FRWheelMR->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		FRWheelMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		FRWheelMR->SetRotationOffset(vec3(90, 0, 0));
		std::shared_ptr<RayCollider> FRWheelCollider = FRWheel->AddComponent<RayCollider>();
		FRWheelCollider->SetDirection(vec3(0, -1, 0));
		FRWheelCollider->SetLength(0.5);
		std::shared_ptr<Rigidbody> FRWheelRB = FRWheel->AddComponent<Rigidbody>();
		FRWheelRB->SetMass(5);
		FRWheelRB->LockRotation(true);
		std::shared_ptr<boxController> FRBC = FRWheel->AddComponent<boxController>();
		FRBC->rb = FRWheelRB;
		FRBC->carBody = carBody;

		// Rear Left Wheel
		std::shared_ptr<Entity> RLWheel = core->AddEntity();
		RLWheel->SetTag("wheel");
		RLWheel->GetComponent<Transform>()->SetPosition(vec3(-1, 0.25, -16.5));
		RLWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		RLWheel->GetComponent<Transform>()->SetScale(vec3(0.5, 0.5, 0.15));
		std::shared_ptr<ModelRenderer> RLWheelMR = RLWheel->AddComponent<ModelRenderer>();
		RLWheelMR->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		RLWheelMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		RLWheelMR->SetRotationOffset(vec3(90, 0, 0));
		std::shared_ptr<RayCollider> RLWheelCollider = RLWheel->AddComponent<RayCollider>();
		RLWheelCollider->SetDirection(vec3(0, -1, 0));
		RLWheelCollider->SetLength(0.5);
		std::shared_ptr<Rigidbody> RLWheelRB = RLWheel->AddComponent<Rigidbody>();
		RLWheelRB->SetMass(5);
		RLWheelRB->LockRotation(true);
		std::shared_ptr<boxController> RLBC = RLWheel->AddComponent<boxController>();
		RLBC->rb = RLWheelRB;
		RLBC->carBody = carBody;

		// Rear Right Wheel
		std::shared_ptr<Entity> RRWheel = core->AddEntity();
		RRWheel->SetTag("wheel");
		RRWheel->GetComponent<Transform>()->SetPosition(vec3(1, 0.25, -16.5));
		RRWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		RRWheel->GetComponent<Transform>()->SetScale(vec3(0.5, 0.5, 0.15));
		std::shared_ptr<ModelRenderer> RRWheelMR = RRWheel->AddComponent<ModelRenderer>();
		RRWheelMR->SetModel(core->GetResources()->Load<Model>("shapes/cylinder"));
		RRWheelMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		RRWheelMR->SetRotationOffset(vec3(90, 0, 0));
		std::shared_ptr<RayCollider> RRWheelCollider = RRWheel->AddComponent<RayCollider>();
		RRWheelCollider->SetDirection(vec3(0, -1, 0));
		RRWheelCollider->SetLength(0.5);
		std::shared_ptr<Rigidbody> RRWheelRB = RRWheel->AddComponent<Rigidbody>();
		RRWheelRB->SetMass(5);
		RRWheelRB->LockRotation(true);
		std::shared_ptr<boxController> RRBC = RRWheel->AddComponent<boxController>();
		RRBC->rb = RRWheelRB;
		RRBC->carBody = carBody;

		// Wheel Anchors
		std::shared_ptr<Entity> FLWheelAnchor = core->AddEntity();
		FLWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(-1, -0.25, 0.5));
		FLWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.1, 0.1, 0.1));
		FLWheelAnchor->GetComponent<Transform>()->SetParent(carBody);
		std::shared_ptr<ModelRenderer> FLWheelAnchorMR = FLWheelAnchor->AddComponent<ModelRenderer>();
		FLWheelAnchorMR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		FLWheelAnchorMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<Suspension> FLWheelAnchorSuspension = FLWheelAnchor->AddComponent<Suspension>();
		FLWheelAnchorSuspension->SetWheel(FLWheel);
		FLWheelAnchorSuspension->SetCarBody(carBody);
		FLWheelAnchorSuspension->SetAnchorPoint(FLWheelAnchor);

		std::shared_ptr<Entity> FRWheelAnchor = core->AddEntity();
		FRWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(1, -0.25, 0.5));
		FRWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.1, 0.1, 0.1));
		FRWheelAnchor->GetComponent<Transform>()->SetParent(carBody);
		std::shared_ptr<ModelRenderer> FRWheelAnchorMR = FRWheelAnchor->AddComponent<ModelRenderer>();
		FRWheelAnchorMR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		FRWheelAnchorMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<Suspension> FRWheelAnchorSuspension = FRWheelAnchor->AddComponent<Suspension>();
		FRWheelAnchorSuspension->SetWheel(FRWheel);
		FRWheelAnchorSuspension->SetCarBody(carBody);
		FRWheelAnchorSuspension->SetAnchorPoint(FRWheelAnchor);

		std::shared_ptr<Entity> RLWheelAnchor = core->AddEntity();
		RLWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(-1, -0.25, -0.5));
		RLWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.1, 0.1, 0.1));
		RLWheelAnchor->GetComponent<Transform>()->SetParent(carBody);
		std::shared_ptr<ModelRenderer> RLWheelAnchorMR = RLWheelAnchor->AddComponent<ModelRenderer>();
		RLWheelAnchorMR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		RLWheelAnchorMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<Suspension> RLWheelAnchorSuspension = RLWheelAnchor->AddComponent<Suspension>();
		RLWheelAnchorSuspension->SetWheel(RLWheel);
		RLWheelAnchorSuspension->SetCarBody(carBody);
		RLWheelAnchorSuspension->SetAnchorPoint(RLWheelAnchor);

		std::shared_ptr<Entity> RRWheelAnchor = core->AddEntity();
		RRWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(1, -0.25, -0.5));
		RRWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.1, 0.1, 0.1));
		RRWheelAnchor->GetComponent<Transform>()->SetParent(carBody);
		std::shared_ptr<ModelRenderer> RRWheelAnchorMR = RRWheelAnchor->AddComponent<ModelRenderer>();
		RRWheelAnchorMR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		RRWheelAnchorMR->SetTexture(core->GetResources()->Load<Texture>("images/cat"));
		std::shared_ptr<Suspension> RRWheelAnchorSuspension = RRWheelAnchor->AddComponent<Suspension>();
		RRWheelAnchorSuspension->SetWheel(RRWheel);
		RRWheelAnchorSuspension->SetCarBody(carBody);
		RRWheelAnchorSuspension->SetAnchorPoint(RRWheelAnchor);

		// Track
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
		testEntity->AddComponent<CollisionTest>()->rb = FLWheelRB;


	}

	core->Run();
}