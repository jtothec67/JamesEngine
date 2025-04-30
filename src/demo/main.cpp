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
			GetTransform()->Move(GetTransform()->GetRight() * speed);// * GetCore()->DeltaTime());
		if (GetKeyboard()->IsKey(SDLK_d))
			GetTransform()->Move(-GetTransform()->GetRight() * speed);// * GetCore()->DeltaTime());
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

		if (GetKeyboard()->IsKeyDown(SDLK_o))
		{
			GetCore()->SetTimeScale(0.1);
		}
		else if (GetKeyboard()->IsKeyDown(SDLK_p))
		{
			GetCore()->SetTimeScale(1);
		}
		else if (GetKeyboard()->IsKeyDown(SDLK_m))
		{
			GetCore()->SetTimeScale(0);
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

	std::shared_ptr<Entity> rearDownforcePos;
	std::shared_ptr<Entity> frontDownforcePos;

	float maxSteeringAngle = 25.f;
	float wheelTurnRate = 30.f;

	float driveTorque = 1600.f;
	float frontBrakeTorque = 2000.f;
	float rearBrakeTorque = 1350.f; // Values at a 60-40 brake bias, seem high but don't shoot the messenger.

	float forwardSpeed = 100000.f;
	float turnSpeed = 1000.f;

	bool lastInputController = false;

	float mSteerDeadzone = 0.1f;

	float mThrottleMaxInput = 0.65f;
	float mThrottleDeadZone = 0.05f;

	float mBrakeMaxInput = 0.79f;
	float mBrakeDeadZone = 0.05f;

	float mThrottleInput = 0.f;
	float mBrakeInput = 0.f;
	float mSteerInput = 0.f;

	// Downforce
	float rearDownforceAt200 = 6000.0f; // Newtons at 200 km/h
	float frontDownforceAt200 = 3500.0f; // Newtons at 200 km/h
	float referenceSpeed = 200.0f / 3.6f; // Convert km/h to m/s

	void OnTick()
	{
		// SDL_CONTROLLER_AXIS_LEFTX is the left stick X axis, -1 left to 1 right
		// SDL_CONTROLLER_AXIS_LEFTY is the left stick Y axis, -1 up to 1 down
		// Same for RIGHT
		//
		// SDL_CONTROLLER_AXIS_TRIGGERLEFT is the left trigger, 0 to 1
		// Same for RIGHT

		if (GetKeyboard()->IsKey(SDLK_h))
		{
			FLWheelSuspension->SetSteeringAngle(maxSteeringAngle);
			FRWheelSuspension->SetSteeringAngle(maxSteeringAngle);
			mSteerInput = maxSteeringAngle;
			lastInputController = false;
		}

		if (GetKeyboard()->IsKey(SDLK_k))
		{
			FLWheelSuspension->SetSteeringAngle(-maxSteeringAngle);
			FRWheelSuspension->SetSteeringAngle(-maxSteeringAngle);
			mSteerInput = -maxSteeringAngle;
			lastInputController = false;
		}

		if (!lastInputController && !GetKeyboard()->IsKey(SDLK_k) && !GetKeyboard()->IsKey(SDLK_h))
		{
			FLWheelSuspension->SetSteeringAngle(0);
			FRWheelSuspension->SetSteeringAngle(0);
			mSteerInput = 0;
		}

		float leftStickX = GetInput()->GetController()->GetAxis(SDL_CONTROLLER_AXIS_LEFTX);
		
		// Remap from [deadzone, 1] to [0, 1]
		float sign = (leftStickX > 0) ? 1.0f : -1.0f;
		float adjusted = (fabs(leftStickX) - mSteerDeadzone) / (1.0f - mSteerDeadzone);
		float deadzonedX = adjusted * sign;
		if (std::fabs(leftStickX) < mSteerDeadzone)
			deadzonedX = 0.f;

		float steerAngle = -maxSteeringAngle * deadzonedX;
		if (!GetKeyboard()->IsKey(SDLK_h) && !GetKeyboard()->IsKey(SDLK_k))
		{
			FLWheelSuspension->SetSteeringAngle(steerAngle);
			FRWheelSuspension->SetSteeringAngle(steerAngle);
			mSteerInput = steerAngle;
			lastInputController = true;
		}

		if (GetKeyboard()->IsKey(SDLK_SPACE))
		{
			SetPosition(vec3(70.0522, 18.086, -144.966));
			SetRotation(vec3(0, 90, 0));
			rb->SetVelocity(vec3(0, 0, 0));
			rb->SetAngularVelocity(vec3(0, 0, 0));
			rb->SetAngularMomentum(vec3(0, 0, 0));
			FLWheelTire->SetWheelAngularVelocity(0);
			FRWheelTire->SetWheelAngularVelocity(0);
			RLWheelTire->SetWheelAngularVelocity(0);
			RRWheelTire->SetWheelAngularVelocity(0);
		}

		//std::cout << "Car forward speed: " << glm::dot(rb->GetVelocity(), GetTransform()->GetForward()) << std::endl;
		//std::cout << "Positioinn: " << GetTransform()->GetPosition().x << ", " << GetTransform()->GetPosition().y << ", " << GetTransform()->GetPosition().z << std::endl;
	}

	void OnFixedTick()
	{
		if (GetKeyboard()->IsKey(SDLK_u))
		{
			RLWheelTire->AddDriveTorque(driveTorque);
			RRWheelTire->AddDriveTorque(driveTorque);
			mThrottleInput = 1;
		}
		else
			mThrottleInput = 0;

		if (GetKeyboard()->IsKey(SDLK_j))
		{
			FLWheelTire->AddBrakeTorque(frontBrakeTorque);
			FRWheelTire->AddBrakeTorque(frontBrakeTorque);
			RLWheelTire->AddBrakeTorque(rearBrakeTorque);
			RRWheelTire->AddBrakeTorque(rearBrakeTorque);
			mBrakeInput = 1;
		}
		else
			mBrakeInput = 0;

		float throttleTrigger = GetInput()->GetController()->GetAxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
		if (throttleTrigger < mThrottleDeadZone)
			throttleTrigger = 0.f;
		else
		{
			if (throttleTrigger > mThrottleMaxInput)
				throttleTrigger = 1.0f;
			else
				throttleTrigger = (throttleTrigger - mThrottleDeadZone) / (mThrottleMaxInput - mThrottleDeadZone);
		}

		float brakeTrigger = GetInput()->GetController()->GetAxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		if (brakeTrigger < mBrakeDeadZone)
			brakeTrigger = 0.0f;
		else
		{
			if (brakeTrigger > mBrakeMaxInput)
				brakeTrigger = 1.0f;
			else
				brakeTrigger = (brakeTrigger - mBrakeDeadZone) / (mBrakeMaxInput - mBrakeDeadZone);
		}

		if (!GetKeyboard()->IsKey(SDLK_u) && !GetKeyboard()->IsKey(SDLK_j))
		{
			RLWheelTire->AddDriveTorque(throttleTrigger * driveTorque);
			RRWheelTire->AddDriveTorque(throttleTrigger * driveTorque);
			mThrottleInput = throttleTrigger;

			FLWheelTire->AddBrakeTorque(brakeTrigger * frontBrakeTorque);
			FRWheelTire->AddBrakeTorque(brakeTrigger * frontBrakeTorque);
			RLWheelTire->AddBrakeTorque(brakeTrigger * rearBrakeTorque);
			RRWheelTire->AddBrakeTorque(brakeTrigger * rearBrakeTorque);
			mBrakeInput = brakeTrigger;
		}

		// Get forward and down directions from car's transform
		glm::vec3 forward = glm::normalize(GetEntity()->GetComponent<Transform>()->GetForward());
		glm::vec3 down = -glm::normalize(GetEntity()->GetComponent<Transform>()->GetUp());

		// Project velocity onto forward direction to get forward speed
		float forwardSpeed = glm::dot(rb->GetVelocity(), forward);

		// Ensure forwardSpeed is non-negative for square
		forwardSpeed = std::max(0.0f, forwardSpeed);

		// Compute downforce scaling factor
		float speedRatio = forwardSpeed / referenceSpeed;
		float scale = speedRatio * speedRatio;

		// Compute actual downforces in car's down direction
		glm::vec3 rearDownforce = down * (rearDownforceAt200 * scale);
		glm::vec3 frontDownforce = down * (frontDownforceAt200 * scale);

		// Apply them to the car at rear and front downforce positions
		rb->ApplyForce(rearDownforce, rearDownforcePos->GetComponent<Transform>()->GetPosition());
		rb->ApplyForce(frontDownforce, frontDownforcePos->GetComponent<Transform>()->GetPosition());

		std::cout << "Car forward speed: " << forwardSpeed * 3.6f << std::endl;

		/*std::cout << "Rear downforce: " << rearDownforce.x << ", " << rearDownforce.y << ", " << rearDownforce.z << std::endl;
		std::cout << "Front downforce: " << frontDownforce.x << ", " << frontDownforce.y << ", " << frontDownforce.z << std::endl;
		std::cout << "Rear downforce magnitude: " << glm::length(rearDownforce) << std::endl;
		std::cout << "Front downforce magnitude: " << glm::length(frontDownforce) << std::endl;*/
	}

	void OnGUI()
	{
		int width, height;
		GetCore()->GetWindow()->GetWindowSize(width, height);

		GetGUI()->Image(vec2(300, 300), vec2(220, 120), GetCore()->GetResources()->Load<Texture>("images/white"));
		GetGUI()->Image(vec2(300, 100), vec2(220, 120), GetCore()->GetResources()->Load<Texture>("images/white"));

		GetGUI()->Image(vec2(width/2, 200), vec2(770, 120), GetCore()->GetResources()->Load<Texture>("images/white"));

		if (mSteerInput > 0)
			GetGUI()->BlendImage(vec2((width / 2) - 750/4, 200), vec2(750/2, 100), GetCore()->GetResources()->Load<Texture>("images/black"), GetCore()->GetResources()->Load<Texture>("images/transparent"), 1 - (mSteerInput / maxSteeringAngle));
		else if (mSteerInput < 0)
			GetGUI()->BlendImage(vec2((width / 2) + 750 / 4, 200), vec2(750 / 2, 100), GetCore()->GetResources()->Load<Texture>("images/transparent"), GetCore()->GetResources()->Load<Texture>("images/black"), (mSteerInput / -maxSteeringAngle));

		GetGUI()->BlendImage(vec2(300, 300), vec2(200, 100), GetCore()->GetResources()->Load<Texture>("images/transparent"), GetCore()->GetResources()->Load<Texture>("images/green"), mThrottleInput);
		GetGUI()->BlendImage(vec2(300, 100), vec2(200, 100), GetCore()->GetResources()->Load<Texture>("images/transparent"), GetCore()->GetResources()->Load<Texture>("images/red"), mBrakeInput);
	
		//GetGUI()->Text(vec2(300, 300), 40, vec3(0, 0, 0), std::to_string(mThrottleInput), GetCore()->GetResources()->Load<Font>("fonts/munro"));
		//GetGUI()->Text(vec2(300, 100), 40, vec3(0, 0, 0), std::to_string(mBrakeInput), GetCore()->GetResources()->Load<Font>("fonts/munro"));
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

	void OnCollision(std::shared_ptr<Entity> _collidedEntity)
	{
		std::cout << "COLLISION" << std::endl;
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
		tyreParams.brushLongStiff = 200000.f;
		tyreParams.brushLatStiff = 180000.f;

		tyreParams.brushLongStiffCoeff = 60;
		tyreParams.brushLatStiffCoeff = 80;

		tyreParams.peakFrictionCoefficient = 1.5f;
		tyreParams.tireRadius = 0.34f;
		tyreParams.wheelMass = 25.f;

		float FStiffness = 30000;
		float FDamping = 7000;

		float RStiffness = 40000;
		float RDamping = 7000;

		core->GetSkybox()->SetTexture(core->GetResources()->Load<SkyboxTexture>("skyboxes/sky"));

		core->GetLightManager()->AddLight("light1", vec3(0, 20, 0), vec3(1, 1, 1), 1.f);
		core->GetLightManager()->SetAmbient(vec3(1.f));

		// Cameras
		std::shared_ptr<Entity> freeCamEntity = core->AddEntity();
		std::shared_ptr<Camera> freeCam = freeCamEntity->AddComponent<Camera>();
		freeCam->SetPriority(10);
		freeCamEntity->GetComponent<Transform>()->SetPosition(vec3(-5, 1, -0));
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
		track->GetComponent<Transform>()->SetPosition(vec3(0, 0, 0));
		track->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		std::shared_ptr<ModelRenderer> trackMR = track->AddComponent<ModelRenderer>();
		trackMR->SetModel(core->GetResources()->Load<Model>("models/Austria/Source/austria5"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/garages_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/garages_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/road_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/grass_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/tarmac_dirty_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/SPIELBERG_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/runoff_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/TARMAC_FLAG_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/white"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/grass_2_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/fence_metal_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/barriers_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/banners_spielberg_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/spielberg_sand_patches_baseColor"));
		trackMR->AddTexture(core->GetResources()->Load<Texture>("models/Austria/Textures/trees_spielberg_baseColor"));
		std::shared_ptr<ModelCollider> trackCollider = track->AddComponent<ModelCollider>();
		trackCollider->SetModel(core->GetResources()->Load<Model>("models/Austria/Source/austria5"));
		trackCollider->SetDebugVisual(false);

		// Car Body
		std::shared_ptr<Entity> carBody = core->AddEntity();
		carBody->SetTag("carBody");
		carBody->GetComponent<Transform>()->SetPosition(vec3(0, 0, 0));
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
		carBodyRB->SetMass(1230);
		//carBodyRB->AddForce(vec3(12300000, 0, 0));
		cockpitCamEntity->GetComponent<Transform>()->SetParent(carBody);
		bonnetCamEntity->GetComponent<Transform>()->SetParent(carBody);
		chaseCamEntity->GetComponent<Transform>()->SetParent(carBody);
		wheelCamEntity->GetComponent<Transform>()->SetParent(carBody);

		std::shared_ptr<Entity> rearDownForcePos = core->AddEntity();
		rearDownForcePos->SetTag("rear downforce pos");
		rearDownForcePos->GetComponent<Transform>()->SetPosition(vec3(0, 0.83, -1.86));
		rearDownForcePos->GetComponent<Transform>()->SetParent(carBody);

		std::shared_ptr<Entity> frontDownForcePos = core->AddEntity();
		frontDownForcePos->SetTag("front downforce pos");
		frontDownForcePos->GetComponent<Transform>()->SetPosition(vec3(0, 0.278, 2.4));
		frontDownForcePos->GetComponent<Transform>()->SetParent(carBody);

		
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
		FLWheelRB->SetMass(2.5);
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
		FRWheelRB->SetMass(2.5);
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
		RLWheelRB->SetMass(2.5);
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
		RRWheelRB->SetMass(2.5);
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
		carController->frontDownforcePos = frontDownForcePos;
		carController->rearDownforcePos = rearDownForcePos;

	}

	core->Run();
}