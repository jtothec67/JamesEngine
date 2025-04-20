#include "JamesEngine/JamesEngine.h"

#include <iostream>

using namespace JamesEngine;

struct freelookCamController : public Component
{
	float normalSpeed = 0.005f;
	float fastSpeed = 0.03f;
	float slowSpeed = 0.0005f;
	float sensitivity = 0.045f;

	void OnTick()
	{
		float speed = 0.f;
		if (GetKeyboard()->IsKey(SDLK_LSHIFT))
			speed = fastSpeed;
		else if (GetKeyboard()->IsKey(SDLK_LCTRL))
			speed = slowSpeed;
		else
			speed = normalSpeed;

		if (GetKeyboard()->IsKey(SDLK_w))
			GetTransform()->Move(-GetTransform()->GetForward() * speed);// * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_s))
			GetTransform()->Move(GetTransform()->GetForward() * speed);// * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_a))
			GetTransform()->Move(-GetTransform()->GetRight() * speed);// * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_d))
			GetTransform()->Move(GetTransform()->GetRight() * speed);// * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_q))
			GetTransform()->Move(-GetTransform()->GetUp() * speed);// * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_e))
			GetTransform()->Move(GetTransform()->GetUp() * speed);// * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_UP))
			GetTransform()->Rotate(vec3(sensitivity, 0, 0));// * GetCore()->DeltaTime(), 0, 0));
		if (GetKeyboard()->IsKey(SDLK_DOWN))
			GetTransform()->Rotate(vec3(-sensitivity, 0, 0));// * GetCore()->DeltaTime(), 0, 0));
		if (GetKeyboard()->IsKey(SDLK_LEFT))
			GetTransform()->Rotate(vec3(0, sensitivity, 0));// * GetCore()->DeltaTime(), 0));
		if (GetKeyboard()->IsKey(SDLK_RIGHT))
			GetTransform()->Rotate(vec3(0, -sensitivity, 0));// * GetCore()->DeltaTime(), 0));
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

	std::shared_ptr<Suspension> FLWheelSuspension;
	std::shared_ptr<Suspension> FRWheelSuspension;

	std::shared_ptr<Tire> FLWheelTire;
	std::shared_ptr<Tire> FRWheelTire;
	std::shared_ptr<Tire> RLWheelTire;
	std::shared_ptr<Tire> RRWheelTire;

	float maxSteeringAngle = 30.f;
	float wheelTurnRate = 30.f;

	float driveTorque = 300.f;
	float frontBrakeTorque = 2000.f;
	float rearBrakeTorque = 1350.f; // Values at a 60-40 brake bias, seem high but don't shoot the messenger.

	float forwardSpeed = 1000.f;
	float turnSpeed = 1000.f;

	void OnTick()
	{
		if (GetKeyboard()->IsKey(SDLK_h))
		{
			float steeringAngle = FLWheelSuspension->GetSteeringAngle();
			steeringAngle += wheelTurnRate * GetCore()->DeltaTime();
			if (steeringAngle > maxSteeringAngle)
				steeringAngle = maxSteeringAngle;
			else if (steeringAngle < -maxSteeringAngle)
				steeringAngle = -maxSteeringAngle;

			FLWheelSuspension->SetSteeringAngle(steeringAngle);
			FRWheelSuspension->SetSteeringAngle(steeringAngle);
		}

		if (GetKeyboard()->IsKey(SDLK_k))
		{
			float steeringAngle = FLWheelSuspension->GetSteeringAngle();
			steeringAngle -= wheelTurnRate * GetCore()->DeltaTime();
			if (steeringAngle > maxSteeringAngle)
				steeringAngle = maxSteeringAngle;
			else if (steeringAngle < -maxSteeringAngle)
				steeringAngle = -maxSteeringAngle;

			FLWheelSuspension->SetSteeringAngle(steeringAngle);
			FRWheelSuspension->SetSteeringAngle(steeringAngle);
		}

		if (GetKeyboard()->IsKey(SDLK_u))
		{
			RLWheelTire->AddDriveTorque(driveTorque);
			RRWheelTire->AddDriveTorque(driveTorque);
		}

		if (GetKeyboard()->IsKey(SDLK_j))
		{
			FLWheelTire->AddBrakeTorque(frontBrakeTorque);
			FRWheelTire->AddBrakeTorque(frontBrakeTorque);
			RLWheelTire->AddBrakeTorque(rearBrakeTorque);
			RRWheelTire->AddBrakeTorque(rearBrakeTorque);
		}


		if (GetKeyboard()->IsKey(SDLK_SPACE))
		{
			SetPosition(vec3(0, 0.8 - 0.45, -16));
			SetRotation(vec3(0, 0, 0));
			rb->SetVelocity(vec3(0, 0, 0));
			rb->SetAngularVelocity(vec3(0, 0, 0));
			rb->SetAngularMomentum(vec3(0, 0, 0));
			FLWheelTire->SetWheelAngularVelocity(0);
			FRWheelTire->SetWheelAngularVelocity(0);
			RLWheelTire->SetWheelAngularVelocity(0);
			RRWheelTire->SetWheelAngularVelocity(0);
		}
	}
};

struct CameraController : Component
{
	std::vector<std::shared_ptr<Camera>> mCameras;
	int mCurrentCamera = 0;

	void OnAlive()
	{
		GetCore()->FindComponents(mCameras);
		std::cout << "Found " << mCameras.size() << " cameras" << std::endl;
	}

	void OnTick()
	{
		if (GetKeyboard()->IsKeyDown(SDLK_TAB))
		{
			mCurrentCamera++;
			if (mCurrentCamera >= mCameras.size())
				mCurrentCamera = 0;
			GetCore()->GetCamera()->SetPriority(1);
			mCameras[mCurrentCamera]->SetPriority(10);
		}
	}

	float mfpsTimer = 0.f;
	int currentFPS = 0;

	void OnGUI()
	{
		mfpsTimer += GetCore()->DeltaTime();

		if (mfpsTimer > 1.f)
		{
			mfpsTimer = 0.f;
			currentFPS = (int)(1.0f / GetCore()->DeltaTime());
		}

		int width, height;
		GetCore()->GetWindow()->GetWindowSize(width, height);

		GetGUI()->Text(vec2(60, height - 20), 40, vec3(0, 1, 0), std::to_string(currentFPS), GetCore()->GetResources()->Load<Font>("fonts/munro"));
	}
};

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(ivec2(1920, 1080));
	core->SetTimeScale(1.f);

	// Scope to ensure the entities aren't being held in main if they're destroyed
	{

		TireParams tyreParams;
		tyreParams.longitudinalStiffness = 50000.f;
		tyreParams.lateralStiffness = 40000.f;
		tyreParams.peakFrictionCoefficient = 1.f;
		tyreParams.tireRadius = 0.34f;
		tyreParams.wheelMass = 25.f;

		float FStiffness = 5000;
		float FDamping = 100;

		float RStiffness = 6000;
		float RDamping = 100;

		core->GetSkybox()->SetTexture(core->GetResources()->Load<SkyboxTexture>("skyboxes/sky"));

		core->GetLightManager()->AddLight("light1", vec3(0, 20, 0), vec3(1, 1, 1), 1.f);
		core->GetLightManager()->SetAmbient(vec3(1.f));

		// Cameras
		std::shared_ptr<Entity> freeCamEntity = core->AddEntity();
		std::shared_ptr<Camera> freeCam = freeCamEntity->AddComponent<Camera>();
		freeCam->SetPriority(10);
		freeCamEntity->GetComponent<Transform>()->SetPosition(vec3(-5, 1, -16));
		freeCamEntity->GetComponent<Transform>()->SetRotation(vec3(0, -90, 0));
		freeCamEntity->AddComponent<CameraController>();
		freeCamEntity->AddComponent<freelookCamController>();

		std::shared_ptr<Entity> cockpitCamEntity = core->AddEntity();
		std::shared_ptr<Camera> cockpitCam = cockpitCamEntity->AddComponent<Camera>();
		cockpitCam->SetPriority(1);
		cockpitCam->SetFov(90.f);
		cockpitCamEntity->GetComponent<Transform>()->SetPosition(vec3(0.371884, 0.62567, -0.148032));
		cockpitCamEntity->GetComponent<Transform>()->SetRotation(vec3(0, 180, 0));

		std::shared_ptr<Entity> bonnetCamEntity = core->AddEntity();
		std::shared_ptr<Camera> bonnetCam = bonnetCamEntity->AddComponent<Camera>();
		bonnetCam->SetPriority(1);
		bonnetCam->SetFov(60.f);
		bonnetCamEntity->GetComponent<Transform>()->SetPosition(vec3(0, 0.767, 0.39));
		bonnetCamEntity->GetComponent<Transform>()->SetRotation(vec3(0, 180, 0));

		std::shared_ptr<Entity> chaseCamEntity = core->AddEntity();
		std::shared_ptr<Camera> chaseCam = chaseCamEntity->AddComponent<Camera>();
		chaseCam->SetPriority(1);
		chaseCamEntity->GetComponent<Transform>()->SetPosition(vec3(0, 1.66, -5.98));
		chaseCamEntity->GetComponent<Transform>()->SetRotation(vec3(0, 180, 0));

		std::shared_ptr<Entity> wheelCamEntity = core->AddEntity();
		std::shared_ptr<Camera> wheelCam = wheelCamEntity->AddComponent<Camera>();
		wheelCam->SetPriority(1);
		wheelCamEntity->GetComponent<Transform>()->SetPosition(vec3(-1.36, 0.246, -1.63));
		wheelCamEntity->GetComponent<Transform>()->SetRotation(vec3(0, 183.735, 0));

		// Track
		std::shared_ptr<Entity> track = core->AddEntity();
		track->SetTag("track");
		track->GetComponent<Transform>()->SetPosition(vec3(0, 0, 10));
		track->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		std::shared_ptr<ModelRenderer> trackMR = track->AddComponent<ModelRenderer>();
		trackMR->SetModel(core->GetResources()->Load<Model>("models/track/cartoon_track_trimmed no-mtl"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/track/rock"));
		std::shared_ptr<ModelCollider> trackCollider = track->AddComponent<ModelCollider>();
		trackCollider->SetModel(core->GetResources()->Load<Model>("models/track/cartoon_track_trimmed no-mtl"));
		trackCollider->SetDebugVisual(false);

		// Car Body
		std::shared_ptr<Entity> carBody = core->AddEntity();
		carBody->SetTag("carBody");
		carBody->GetComponent<Transform>()->SetPosition(vec3(0, 0.8-0.45, -16));
		carBody->GetComponent<Transform>()->SetRotation(vec3(0, 90, 0));
		carBody->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> mercedesMR = carBody->AddComponent<ModelRenderer>();
		mercedesMR->SetRotationOffset(vec3(0, 180, 0));
		mercedesMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/source/mercedes"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_0"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_3"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_2"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_5"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_7"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_9"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_2"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_11"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_13"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_15"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_2"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_2"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_9"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_2"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_11"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_22"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_13"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_24"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_15"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_0"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_26"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_28"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_28"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_30")); // Car Exterior
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_33"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_34"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_37"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_37"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/gltf_embedded_39"));
		mercedesMR->AddTexture(core->GetResources()->Load<Texture>("models/Mercedes/textures/mirrors"));
		std::shared_ptr<BoxCollider> carBodyCollider = carBody->AddComponent<BoxCollider>();
		carBodyCollider->SetSize(vec3(1.97, 0.9, 4.52));
		carBodyCollider->SetPositionOffset(vec3(0, 0.37, 0.22));
		std::shared_ptr<Rigidbody> carBodyRB = carBody->AddComponent<Rigidbody>();
		carBodyRB->SetMass(123);
		cockpitCamEntity->GetComponent<Transform>()->SetParent(carBody);
		bonnetCamEntity->GetComponent<Transform>()->SetParent(carBody);
		chaseCamEntity->GetComponent<Transform>()->SetParent(carBody);
		wheelCamEntity->GetComponent<Transform>()->SetParent(carBody);
		
		// Wheel Anchors
		std::shared_ptr<Entity> FLWheelAnchor = core->AddEntity();
		FLWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(0.856, -0.0079, 1.6));
		FLWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.01, 0.01, 0.01));
		FLWheelAnchor->GetComponent<Transform>()->SetParent(carBody);
		std::shared_ptr<ModelRenderer> FLWheelAnchorMR = FLWheelAnchor->AddComponent<ModelRenderer>();
		FLWheelAnchorMR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		FLWheelAnchorMR->AddTexture(core->GetResources()->Load<Texture>("images/cat"));

		std::shared_ptr<Entity> FRWheelAnchor = core->AddEntity();
		FRWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(-0.856, -0.0079, 1.6));
		FRWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.01, 0.01, 0.01));
		FRWheelAnchor->GetComponent<Transform>()->SetParent(carBody);
		std::shared_ptr<ModelRenderer> FRWheelAnchorMR = FRWheelAnchor->AddComponent<ModelRenderer>();
		FRWheelAnchorMR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		FRWheelAnchorMR->AddTexture(core->GetResources()->Load<Texture>("images/cat"));

		std::shared_ptr<Entity> RLWheelAnchor = core->AddEntity();
		RLWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(0.863, -0.0009, -1.027));
		RLWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.01, 0.01, 0.01));
		RLWheelAnchor->GetComponent<Transform>()->SetParent(carBody);
		std::shared_ptr<ModelRenderer> RLWheelAnchorMR = RLWheelAnchor->AddComponent<ModelRenderer>();
		RLWheelAnchorMR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		RLWheelAnchorMR->AddTexture(core->GetResources()->Load<Texture>("images/cat"));

		std::shared_ptr<Entity> RRWheelAnchor = core->AddEntity();
		RRWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(-0.863, -0.0009, -1.027));
		RRWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.01, 0.01, 0.01));
		RRWheelAnchor->GetComponent<Transform>()->SetParent(carBody);
		std::shared_ptr<ModelRenderer> RRWheelAnchorMR = RRWheelAnchor->AddComponent<ModelRenderer>();
		RRWheelAnchorMR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		RRWheelAnchorMR->AddTexture(core->GetResources()->Load<Texture>("images/cat"));


		// Front Left Wheel
		std::shared_ptr<Entity> FLWheel = core->AddEntity();
		FLWheel->SetTag("FLwheel");
		FLWheel->GetComponent<Transform>()->SetPosition(vec3(0.8767, 0.793- 0.45, -14.3998));
		FLWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		FLWheel->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> FLWheelMR = FLWheel->AddComponent<ModelRenderer>();
		FLWheelMR->SetModel(core->GetResources()->Load<Model>("models/MercedesWheels/source/Wheels"));
		FLWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_17"));
		FLWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_19"));
		FLWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_31"));
		FLWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_35"));
		FLWheelMR->SetRotationOffset(vec3(0, 90, 0));
		std::shared_ptr<RayCollider> FLWheelCollider = FLWheel->AddComponent<RayCollider>();
		FLWheelCollider->SetDirection(vec3(0, -1, 0));
		FLWheelCollider->SetLength(0.4);
		std::shared_ptr<Rigidbody> FLWheelRB = FLWheel->AddComponent<Rigidbody>();
		//FLWheelRB->SetMass(0.1);
		FLWheelRB->SetMass(25);
		FLWheelRB->LockRotation(true);
		FLWheelRB->IsStatic(true);
		std::shared_ptr<Suspension> FLWheelSuspension = FLWheel->AddComponent<Suspension>();
		FLWheelSuspension->SetWheel(FLWheel);
		FLWheelSuspension->SetCarBody(carBody);
		FLWheelSuspension->SetAnchorPoint(FLWheelAnchor);
		FLWheelSuspension->SetStiffness(FStiffness);
		FLWheelSuspension->SetDamping(FDamping);
		std::shared_ptr<Tire> FLWheelTire = FLWheel->AddComponent<Tire>();
		FLWheelTire->SetCarBody(carBody);
		FLWheelTire->SetAnchorPoint(FLWheelAnchor);
		FLWheelTire->SetTireParams(tyreParams);
		FLWheelTire->SetInitialRotationOffset(vec3(0, 90, 0));

		// Front Right Wheel
		std::shared_ptr<Entity> FRWheel = core->AddEntity();
		FRWheel->SetTag("FRwheel");
		FRWheel->GetComponent<Transform>()->SetPosition(vec3(-0.8767, 0.793- 0.45, -14.3998));
		FRWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		FRWheel->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> FRWheelMR = FRWheel->AddComponent<ModelRenderer>();
		FRWheelMR->SetModel(core->GetResources()->Load<Model>("models/MercedesWheels/source/Wheels"));
		FRWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_17"));
		FRWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_19"));
		FRWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_31"));
		FRWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_35"));
		FRWheelMR->SetRotationOffset(vec3(0, -90, 0));
		std::shared_ptr<RayCollider> FRWheelCollider = FRWheel->AddComponent<RayCollider>();
		FRWheelCollider->SetDirection(vec3(0, -1, 0));
		FRWheelCollider->SetLength(0.4);
		std::shared_ptr<Rigidbody> FRWheelRB = FRWheel->AddComponent<Rigidbody>();
		//FRWheelRB->SetMass(0.1);
		FRWheelRB->SetMass(25);
		FRWheelRB->LockRotation(true);
		FRWheelRB->IsStatic(true);
		std::shared_ptr<Suspension> FRWheelSuspension = FRWheel->AddComponent<Suspension>();
		FRWheelSuspension->SetWheel(FRWheel);
		FRWheelSuspension->SetCarBody(carBody);
		FRWheelSuspension->SetAnchorPoint(FRWheelAnchor);
		FRWheelSuspension->SetStiffness(FStiffness);
		FRWheelSuspension->SetDamping(FDamping);
		std::shared_ptr<Tire> FRWheelTire = FRWheel->AddComponent<Tire>();
		FRWheelTire->SetCarBody(carBody);
		FRWheelTire->SetAnchorPoint(FRWheelAnchor);
		FRWheelTire->SetTireParams(tyreParams);
		FRWheelTire->SetInitialRotationOffset(vec3(0, -90, 0));

		// Rear Left Wheel
		std::shared_ptr<Entity> RLWheel = core->AddEntity();
		RLWheel->SetTag("RLwheel");
		RLWheel->GetComponent<Transform>()->SetPosition(vec3(0.8767, 0.8003- 0.45, -17.025));
		RLWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		RLWheel->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> RLWheelMR = RLWheel->AddComponent<ModelRenderer>();
		RLWheelMR->SetModel(core->GetResources()->Load<Model>("models/MercedesWheels/source/Wheels"));
		RLWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_17"));
		RLWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_19"));
		RLWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_31"));
		RLWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_35"));
		RLWheelMR->SetRotationOffset(vec3(0, 90, 0));
		std::shared_ptr<RayCollider> RLWheelCollider = RLWheel->AddComponent<RayCollider>();
		RLWheelCollider->SetDirection(vec3(0, -1, 0));
		RLWheelCollider->SetLength(0.4);
		std::shared_ptr<Rigidbody> RLWheelRB = RLWheel->AddComponent<Rigidbody>();
		//RLWheelRB->SetMass(0.1);
		RLWheelRB->SetMass(25);
		RLWheelRB->LockRotation(true);
		RLWheelRB->IsStatic(true);
		std::shared_ptr<Suspension> RLWheelSuspension = RLWheel->AddComponent<Suspension>();
		RLWheelSuspension->SetWheel(RLWheel);
		RLWheelSuspension->SetCarBody(carBody);
		RLWheelSuspension->SetAnchorPoint(RLWheelAnchor);
		RLWheelSuspension->SetStiffness(RStiffness);
		RLWheelSuspension->SetDamping(RDamping);
		std::shared_ptr<Tire> RLWheelTire = RLWheel->AddComponent<Tire>();
		RLWheelTire->SetCarBody(carBody);
		RLWheelTire->SetAnchorPoint(RLWheelAnchor);
		RLWheelTire->SetTireParams(tyreParams);
		RLWheelTire->SetInitialRotationOffset(vec3(0, 90, 0));

		// Rear Right Wheel
		std::shared_ptr<Entity> RRWheel = core->AddEntity();
		RRWheel->SetTag("RRwheel");
		RRWheel->GetComponent<Transform>()->SetPosition(vec3(-0.8767, 0.8003- 0.45, -17.025));
		RRWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		RRWheel->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> RRWheelMR = RRWheel->AddComponent<ModelRenderer>();
		RRWheelMR->SetModel(core->GetResources()->Load<Model>("models/MercedesWheels/source/Wheels"));
		RRWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_17"));
		RRWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_19"));
		RRWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_31"));
		RRWheelMR->AddTexture(core->GetResources()->Load<Texture>("models/MercedesWheels/textures/gltf_embedded_35"));
		RRWheelMR->SetRotationOffset(vec3(0, -90, 0));
		std::shared_ptr<RayCollider> RRWheelCollider = RRWheel->AddComponent<RayCollider>();
		RRWheelCollider->SetDirection(vec3(0, -1, 0));
		RRWheelCollider->SetLength(0.4);
		std::shared_ptr<Rigidbody> RRWheelRB = RRWheel->AddComponent<Rigidbody>();
		//RRWheelRB->SetMass(0.1);
		RRWheelRB->SetMass(25);
		RRWheelRB->LockRotation(true);
		RRWheelRB->IsStatic(true);
		std::shared_ptr<Suspension> RRWheelSuspension = RRWheel->AddComponent<Suspension>();
		RRWheelSuspension->SetWheel(RRWheel);
		RRWheelSuspension->SetCarBody(carBody);
		RRWheelSuspension->SetAnchorPoint(RRWheelAnchor);
		RRWheelSuspension->SetStiffness(RStiffness);
		RRWheelSuspension->SetDamping(RDamping);
		std::shared_ptr<Tire> RRWheelTire = RRWheel->AddComponent<Tire>();
		RRWheelTire->SetCarBody(carBody);
		RRWheelTire->SetAnchorPoint(RRWheelAnchor);
		RRWheelTire->SetTireParams(tyreParams);
		RRWheelTire->SetInitialRotationOffset(vec3(0, -90, 0));

		std::shared_ptr<CarController> carController = carBody->AddComponent<CarController>();
		carController->rb = carBodyRB;
		carController->FLWheelSuspension = FLWheelSuspension;
		carController->FRWheelSuspension = FRWheelSuspension;
		carController->FLWheelTire = FLWheelTire;
		carController->FRWheelTire = FRWheelTire;
		carController->RLWheelTire = RLWheelTire;
		carController->RRWheelTire = RRWheelTire;

		std::shared_ptr<Entity> testEntity = core->AddEntity();
		testEntity->GetComponent<Transform>()->SetPosition(vec3(4.80949, 9.48961, 6.23224));
		testEntity->GetComponent<Transform>()->SetRotation(vec3(0, 90, 0));
		testEntity->GetComponent<Transform>()->SetScale(vec3(0.1, 0.1, 0.1));
		std::shared_ptr<ModelRenderer> testTR = testEntity->AddComponent<ModelRenderer>();
		testTR->SetModel(core->GetResources()->Load<Model>("shapes/sphere"));
		testTR->AddTexture(core->GetResources()->Load<Texture>("images/cat"));
		testEntity->AddComponent<CollisionTest>()->rb = FLWheelRB;


	}

	core->Run();
}