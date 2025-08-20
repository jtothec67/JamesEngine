#include "JamesEngine/JamesEngine.h"

#include "Tire.h"
#include "Suspension.h"
#include "Engine.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include <iostream>
#include <iomanip>

using namespace JamesEngine;

struct freelookCamController : public Component
{
	float normalSpeed = 1.f;
	float fastSpeed = 3.f;
	float slowSpeed = 0.5f;
	float sensitivity = 0.25f;

	void OnTick()
	{
		float speed = 0.f;
		if (GetKeyboard()->IsKey(SDLK_LSHIFT))
			speed = fastSpeed;
		else if (GetKeyboard()->IsKey(SDLK_LCTRL))
			speed = slowSpeed;
		else
			speed = normalSpeed;

		if (GetKeyboard()->IsKey(SDLK_u))
			GetTransform()->Move(-GetTransform()->GetForward() * speed * GetCore()->GetLastFrameTime());
		if (GetKeyboard()->IsKey(SDLK_j))
			GetTransform()->Move(GetTransform()->GetForward() * speed * GetCore()->GetLastFrameTime());
		if (GetKeyboard()->IsKey(SDLK_h))
			GetTransform()->Move(GetTransform()->GetRight() * speed * GetCore()->GetLastFrameTime());
		if (GetKeyboard()->IsKey(SDLK_k))
			GetTransform()->Move(-GetTransform()->GetRight() * speed * GetCore()->GetLastFrameTime());
		if (GetKeyboard()->IsKey(SDLK_y))
			GetTransform()->Move(-GetTransform()->GetUp() * speed * GetCore()->GetLastFrameTime());
		if (GetKeyboard()->IsKey(SDLK_i))
			GetTransform()->Move(GetTransform()->GetUp() * speed * GetCore()->GetLastFrameTime());
		if (GetKeyboard()->IsKey(SDLK_UP))
			GetTransform()->Rotate(vec3(sensitivity, 0, 0 * GetCore()->GetLastFrameTime()));
		if (GetKeyboard()->IsKey(SDLK_DOWN))
			GetTransform()->Rotate(vec3(-sensitivity, 0, 0 * GetCore()->GetLastFrameTime()));
		if (GetKeyboard()->IsKey(SDLK_LEFT))
			GetTransform()->Rotate(vec3(0, sensitivity, 0 * GetCore()->GetLastFrameTime()));
		if (GetKeyboard()->IsKey(SDLK_RIGHT))
			GetTransform()->Rotate(vec3(0, -sensitivity, 0 * GetCore()->GetLastFrameTime()));
	}
};

struct StartFinishLine : public Component
{
	bool onALap = false;

	float lapTime = 0.f;

	float lastLapTime = 0.f;
	std::string lastLapTimeString = "0:00.000";

	float bestLapTime = 0.f;
	std::string bestLapTimeString = "0:00.000";

	float recordSamplesEvery = 0.25f;

	struct sample
	{
		float timestamp;
		glm::vec3 position;
	};

	std::vector<sample> currentLapSamples;
	std::vector<sample> fastestLapSamples;

	float currentDelta = 0.f;
	std::string currentDeltaString = "00.000";

	float sampleTimer = 0.f;

	int lastSampleIndex = 0;

	std::shared_ptr<Entity> car;

	void ResetCar()
	{
		onALap = false;
		lapTime = 0.f;

		currentLapSamples.clear();
		currentDelta = 0.f;
		lastSampleIndex = 0;
	}

	void OnTick()
	{
		if (!onALap)
		{
			lapTime = 0.f;
			return;
		}

		lapTime += GetCore()->DeltaTime();

		sampleTimer += GetCore()->DeltaTime();
		if (sampleTimer >= recordSamplesEvery)
		{
			sampleTimer = 0.f;
			sample s;
			s.timestamp = lapTime;
			s.position = car->GetComponent<Transform>()->GetPosition();
			currentLapSamples.push_back(s);

			if (!fastestLapSamples.empty())
			{
				glm::vec3 currentPos = car->GetComponent<Transform>()->GetPosition();

				float bestT = 0.0f;
				float minDistSq = std::numeric_limits<float>::max();
				float interpolatedTime = 0.0f;

				// Clamp starting index to valid range
				int start = glm::clamp(lastSampleIndex - 2, 0, (int)(fastestLapSamples.size()) - 2);
				int end = glm::min(start + 10, (int)(fastestLapSamples.size()) - 2); // Look ahead a few samples

				for (int i = start; i <= end; ++i)
				{
					const sample& a = fastestLapSamples[i];
					const sample& b = fastestLapSamples[i + 1];

					// Direction vector of the segment
					glm::vec3 seg = b.position - a.position;
					glm::vec3 toCar = currentPos - a.position;

					float segLenSq = glm::dot(seg, seg);
					if (segLenSq == 0.0f) continue; // Skip zero-length segments

					// Project point onto the segment (clamp between 0 and 1)
					float t = glm::clamp(glm::dot(toCar, seg) / segLenSq, 0.0f, 1.0f);

					glm::vec3 projPoint = a.position + t * seg;
					float distSq = glm::distance2(currentPos, projPoint);

					if (distSq < minDistSq)
					{
						minDistSq = distSq;
						bestT = t;
						interpolatedTime = glm::mix(a.timestamp, b.timestamp, t);
						lastSampleIndex = i;
					}
				}

				currentDelta = lapTime - interpolatedTime;
			}
		}
	}

	void OnCollision(std::shared_ptr<Entity> _collidedEntity)
	{
		if (_collidedEntity->GetTag() != "carBody")
			return;

		// 2 second cool down so we only start lap once, can get rid after adding sectors
		if (onALap && lapTime > 2)
		{
			lastLapTime = lapTime;
			lastLapTimeString = FormatLapTime(lastLapTime);

			if (lapTime < bestLapTime || bestLapTime == 0)
			{
				bestLapTime = lastLapTime;
				bestLapTimeString = FormatLapTime(bestLapTime);

				fastestLapSamples = currentLapSamples; // Store the fastest lap samples
			}

			lapTime = 0.f;

			currentLapSamples.clear();
			lastSampleIndex = 0;
		}
		else
		{
			lapTime = 0.f;
			onALap = true;

			// Predict how many samples we will take in a lap, reserve space for efficiency
			currentLapSamples.reserve(110 / recordSamplesEvery);
			fastestLapSamples.reserve(110 / recordSamplesEvery);
		}
	}

	void OnGUI()
	{
		int width, height;
		GetCore()->GetWindow()->GetWindowSize(width, height);

		GetGUI()->Text(vec2(width / 2, height - 50), 50, vec3(0, 0, 0), FormatLapTime(lapTime), GetCore()->GetResources()->Load<Font>("fonts/munro"));

		glm::vec3 deltaColour;
		if (currentDelta == 0.0f)
			deltaColour = vec3(0, 0, 0); // white
		else if (currentDelta < 0.0f)
			deltaColour = vec3(0, 1, 0); // Green
		else
			deltaColour = vec3(1, 0, 0); // Red

		GetGUI()->Text(vec2(width / 2, height - 100), 40, deltaColour, FormatDeltaTime(currentDelta), GetCore()->GetResources()->Load<Font>("fonts/munro"));

		GetGUI()->Text(vec2(width - 200, height - 50), 40, vec3(0, 0, 0), "Last lap: \n" + lastLapTimeString, GetCore()->GetResources()->Load<Font>("fonts/munro"));

		GetGUI()->Text(vec2(width - 200, height - 200), 40, vec3(0, 0, 0), "Best lap: \n" + bestLapTimeString, GetCore()->GetResources()->Load<Font>("fonts/munro"));

		// For shadow map debugging
		//GetGUI()->Image(vec2(width-300, height-300), vec2(600, 600), GetCore()->GetResources()->Load<Texture>("images/mouse"));
	}

	std::string FormatLapTime(float time)
	{
		int minutes = static_cast<int>(time) / 60;
		int seconds = static_cast<int>(time) % 60;
		int milliseconds = static_cast<int>((time - static_cast<int>(time)) * 1000);
		std::ostringstream formattedTime;
		formattedTime << minutes << ":"
			<< std::setw(2) << std::setfill('0') << seconds << "."
			<< std::setw(3) << std::setfill('0') << milliseconds;
		return formattedTime.str();
	}

	std::string FormatDeltaTime(float delta)
	{
		char sign = (delta < 0.0f) ? '-' : '+';
		float absTime = std::fabs(delta);

		std::ostringstream stream;
		stream << sign << std::fixed << std::setprecision(3) << absTime;

		return stream.str();
	}
};

struct CarController : public Component
{
	// Core physics and wheel components
	std::shared_ptr<Rigidbody> rb;
	std::shared_ptr<Suspension> FLWheelSuspension;
	std::shared_ptr<Suspension> FRWheelSuspension;
	std::shared_ptr<Suspension> RLWheelSuspension;
	std::shared_ptr<Suspension> RRWheelSuspension;
	std::shared_ptr<Tire> FLWheelTire;
	std::shared_ptr<Tire> FRWheelTire;
	std::shared_ptr<Tire> RLWheelTire;
	std::shared_ptr<Tire> RRWheelTire;

	std::shared_ptr<Entity> steeringWheel;

	// Downforce anchor points
	std::shared_ptr<Entity> rearDownforcePos;
	std::shared_ptr<Entity> frontDownforcePos;

	// Steering parameters
	float maxTireSteeringAngle = 25.f; // Maximum tire steering angle
	float maxSteeringRotation = 360.f; // Maximum steering wheel rotation

	//// Engine and transmission parameters
	//std::vector<std::pair<float, float>> torqueCurve = {
	//{1000, 250.0f},
	//{1500, 400.0f},
	//{2000, 500.0f},
	//{2500, 600.0f},
	//{3000, 620.0f},
	//{3500, 630.0f},
	//{4000, 640.0f},
	//{4500, 645.0f},
	//{4800, 650.0f},  // Peak torque
	//{5000, 645.0f},
	//{5500, 630.0f},
	//{6000, 610.0f},
	//{6500, 580.0f},
	//{7000, 540.0f},
	//{7500, 480.0f},
	//{8000, 400.0f},
	//{8500, 300.0f}  // Redline
	//};
	float currentRPM = 0;
	float maxRPM = 8000;
	float idleRPM = 1000;
	//int numGears = 6;
	//int currentGear = 1;
	//float gearRatios[6] = { 3, 2.25, 1.75, 1.35, 1.1, 0.9 }; // TEST IN ACC, DRIVE AT SAME RPM IN EACH GEAR AND SEE SPEEDS TO WORK OUT GEAR RATIO!!
	//float finalDrive = 3.7f;
	//float drivetrainEfficiency = 0.8f;

	Engine mEngine;

	float clutchEngagement = 1.0f; // 0 = fully disengaged, 1 = fully engaged
	float bitePointStart = 0.35f; // Minimum engagement to prevent stalling
	float bitePointEnd = 0.65f; // Maximum engagement
	bool autoClutchEnabled = true;
	enum class LaunchState { PreLaunch, Hold, Release } launchState = LaunchState::PreLaunch;

	// Brake torques
	float brakeTorqueCapacity = 15000.f;
	float brakeBias = 0.60f; // 60% front, 40% rear

	// Input tracking
	bool lastInputController = false;

	// Steering and pedal deadzones and limits
	float mSteerDeadzone = 0.15f;
	float mThrottleMaxInput = 0.62f;
	float mThrottleDeadZone = 0.05f;
	float mBrakeMaxInput = 0.75f;
	float mBrakeDeadZone = 0.05f;

	// Current input values
	float mThrottleInput = 0.f;
	float mBrakeInput = 0.f;
	float mSteerInput = 0.f;

	// Aerodynamic properties
	float dragCoefficient = 0.475f;
	float frontalArea = 1.8f; // m^2

	// Downforce settings at reference speed
	//float rearDownforceAt200 = 5300.0f; // N at 200 km/h
	//float frontDownforceAt200 = 4000.0f; // N at 200 km/h
	float rearDownforceAt200 = 1900.0f; // N at 200 km/h
	float frontDownforceAt200 = 2600.0f; // N at 200 km/h
	float referenceSpeed = 200.0f / 3.6f; // m/s

	// Engine audio
	std::shared_ptr<AudioSource> engineAudioSource;

	bool vsyncStatus = false;

	void OnAlive()
	{
		// Initialize engine audio source and looping sound
		engineAudioSource = GetEntity()->AddComponent<AudioSource>();
		engineAudioSource->SetSound(GetCore()->GetResources()->Load<Sound>("sounds/engine sample"));
		engineAudioSource->SetLooping(true);

		// For menu
		vsyncStatus = GetCore()->GetWindow()->GetVSync();

		EngineParams params;
		params.maxRPM = maxRPM;
		params.idleRPM = idleRPM;
		params.gearRatios = { 3, 2.25, 1.75, 1.35, 1.1, 0.9 }; // TEST IN ACC, DRIVE AT SAME RPM IN EACH GEAR AND SEE SPEEDS TO WORK OUT GEAR RATIO!!
		params.torqueCurve = {
			{1000, 250.0f},
			{1500, 400.0f},
			{2000, 500.0f},
			{2500, 600.0f},
			{3000, 620.0f},
			{3500, 630.0f},
			{4000, 640.0f},
			{4500, 645.0f},
			{4800, 650.0f},
			{5000, 645.0f},
			{5500, 630.0f},
			{6000, 610.0f},
			{6500, 580.0f},
			{7000, 540.0f},
			{7500, 480.0f},
			{8000, 400.0f},
			{8500, 300.0f}
		};
		params.finalDrive = 3.7f;
		params.drivetrainEfficiency = 0.8f;

		mEngine.SetEngineParams(params);
	}

	bool clutchReadyToLaunch = true;

	void OnTick()
	{
		// SDL_CONTROLLER_AXIS_LEFTX is the left stick X axis, -1 left to 1 right
		// SDL_CONTROLLER_AXIS_LEFTY is the left stick Y axis, -1 up to 1 down
		// Same for RIGHT
		//
		// SDL_CONTROLLER_AXIS_TRIGGERLEFT is the left trigger, 0 to 1
		// Same for RIGHT

		if (!autoClutchEnabled)
		{
			if (GetInput()->GetController()->IsButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN))
			{
				clutchEngagement = 0.0f; // Disengage clutch when down on dpad
			}
			else
			{
				clutchEngagement = 1.0f; // Engage clutch when not down on dpad
			}
		}

		// Upshift
		if (GetInput()->GetController()->IsButtonDown(SDL_CONTROLLER_BUTTON_X) || GetKeyboard()->IsKeyDown(SDLK_p)) // Square on playstation
		{
			mEngine.SetGear(mEngine.GetCurrentGear() + 1); // Could just be "Go up a gear", but could add gear shifting logic later
		}

		//Downshift
		if (GetInput()->GetController()->IsButtonDown(SDL_CONTROLLER_BUTTON_A) || GetKeyboard()->IsKeyDown(SDLK_o)) // X on playstation
		{
			mEngine.SetGear(mEngine.GetCurrentGear() - 1);
		}

		float wheelAngularVelocity = (RRWheelTire->GetWheelAngularVelocity() + RLWheelTire->GetWheelAngularVelocity()) / 2;
		float wheelRPM = glm::degrees(wheelAngularVelocity) / 6.0f;

		// Update engine
		mEngine.EngineUpdate(mThrottleInput, clutchEngagement, wheelRPM, GetCore()->DeltaTime());

		currentRPM = mEngine.GetRPM();
		clutchEngagement = mEngine.GetClutch();

		// Steer left keyboard
		if (GetKeyboard()->IsKey(SDLK_a))
		{
			FLWheelSuspension->SetSteeringAngle(maxTireSteeringAngle);
			FRWheelSuspension->SetSteeringAngle(maxTireSteeringAngle);
			mSteerInput = maxTireSteeringAngle;
			lastInputController = false;
		}

		// Steer right keyboard
		if (GetKeyboard()->IsKey(SDLK_d))
		{
			FLWheelSuspension->SetSteeringAngle(-maxTireSteeringAngle);
			FRWheelSuspension->SetSteeringAngle(-maxTireSteeringAngle);
			mSteerInput = -maxTireSteeringAngle;
			lastInputController = false;
		}

		// Not on controller and not holding a key, don't steer
		if (!lastInputController && !GetKeyboard()->IsKey(SDLK_d) && !GetKeyboard()->IsKey(SDLK_a))
		{
			FLWheelSuspension->SetSteeringAngle(0);
			FRWheelSuspension->SetSteeringAngle(0);
			mSteerInput = 0;
		}

		// Get steer angle from controller
		float leftStickX = GetInput()->GetController()->GetAxis(SDL_CONTROLLER_AXIS_LEFTX);

		// Remap from [deadzone, 1] to [0, 1]
		float sign = (leftStickX > 0) ? 1.0f : -1.0f;
		float adjusted = (fabs(leftStickX) - mSteerDeadzone) / (1.0f - mSteerDeadzone);
		float deadzonedX = adjusted * sign;
		if (std::fabs(leftStickX) < mSteerDeadzone)
			deadzonedX = 0.f;

		float steerAngle = -maxTireSteeringAngle * deadzonedX;

		// If no keyboard input, steer with controller angle
		if (!GetKeyboard()->IsKey(SDLK_a) && !GetKeyboard()->IsKey(SDLK_d))
		{
			FLWheelSuspension->SetSteeringAngle(steerAngle);
			FRWheelSuspension->SetSteeringAngle(steerAngle);
			mSteerInput = steerAngle;
			lastInputController = true;
		}

		// Change steering wheel rotation based on tire steer angle
		float steerRatio = glm::clamp(mSteerInput / maxTireSteeringAngle, -1.0f, 1.0f);
		float steerRotation = steerRatio * (maxSteeringRotation / 2.f);
		glm::vec3 currentSteerRotation = steeringWheel->GetComponent<Transform>()->GetRotation();
		steeringWheel->GetComponent<Transform>()->SetRotation(vec3(currentSteerRotation.x, currentSteerRotation.y, -steerRotation));

		if (GetKeyboard()->IsKey(SDLK_s))
		{
			mBrakeInput = 1.0f;
			lastInputController = false;
		}
		else
		{
			float b = GetInput()->GetController()->GetAxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
			if (b < mBrakeDeadZone) b = 0.0f;
			else if (b > mBrakeMaxInput) b = 1.0f;
			else b = (b - mBrakeDeadZone) / (mBrakeMaxInput - mBrakeDeadZone);

			mBrakeInput = b;
			if (b > 0.0f) lastInputController = true;
		}

		if (GetKeyboard()->IsKey(SDLK_w))
		{
			mThrottleInput = 1.0f;
			lastInputController = false;
		}
		else
		{
			float t = GetInput()->GetController()->GetAxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
			if (t < mThrottleDeadZone) t = 0.0f;
			else if (t > mThrottleMaxInput) t = 1.0f;
			else t = (t - mThrottleDeadZone) / (mThrottleMaxInput - mThrottleDeadZone);

			mThrottleInput = t;
			if (t > 0.0f) lastInputController = true;
		}

		// Set engine volume to 0 when in menu
		if (menuState == MenuState::Closed)
			engineAudioSource->SetGain(1);
		else
			engineAudioSource->SetGain(0);

		// Change engine pitch based on RPM
		float minPitch = 0.5f;
		float maxPitch = 1.3f;
		if (currentRPM < 6000)
		{
			float t = (currentRPM - idleRPM) / (6000 - idleRPM);
			engineAudioSource->SetPitch(minPitch + t * (1.0f - minPitch));
		}
		else
		{
			float t = (currentRPM - 6000) / (maxRPM - 6000);
			engineAudioSource->SetPitch(1.0f + t * (maxPitch - 1.0f));
		}

		// Reset car
		if (GetKeyboard()->IsKey(SDLK_SPACE))
		{
			SetPosition(vec3(647.479, -65.4695, -252.504));
			SetRotation(vec3(177.438, 48.71, -179.937));
			rb->SetVelocity(vec3(0, 0, 0));
			rb->SetAngularVelocity(vec3(0, 0, 0));
			rb->SetAngularMomentum(vec3(0, 0, 0));
			FLWheelTire->SetWheelAngularVelocity(0);
			FRWheelTire->SetWheelAngularVelocity(0);
			RLWheelTire->SetWheelAngularVelocity(0);
			RRWheelTire->SetWheelAngularVelocity(0);

			GetCore()->FindComponent<StartFinishLine>()->ResetCar(); 
		}
	}

	void OnFixedTick()
	{
		// Compute slip ratio for each tire and total rumble intensity
		float FLSlide = glm::clamp(FLWheelTire->GetSlidingAmount(), 0.5f, 1.f);
		float FRSlide = glm::clamp(FRWheelTire->GetSlidingAmount(), 0.5f, 1.f);
		float RLSlide = glm::clamp(RLWheelTire->GetSlidingAmount(), 0.5f, 1.f);
		float RRSlide = glm::clamp(RRWheelTire->GetSlidingAmount(), 0.5f, 1.f);

		float lowFreq = FLSlide + FRSlide + RLSlide + RRSlide;

		// Apply controller rumble based on average slip
		GetInput()->GetController()->SetRumble((lowFreq / 4) - 0.5f, 0, GetCore()->FixedDeltaTime());


		// Engine torque to wheels
		float wheelTorque = mEngine.GetWheelTorque();

		// Apply wheel torque (/2 split between 2 wheels)
		RLWheelTire->AddDriveTorque(wheelTorque / 2);
		RRWheelTire->AddDriveTorque(wheelTorque / 2);


		// Brake torque to wheels
		float totalBrakeTorque = brakeTorqueCapacity * mBrakeInput;

		float frontBrakeTorque = (totalBrakeTorque * brakeBias) / 2;
		float rearBrakeTorque = (totalBrakeTorque * (1.0f - brakeBias)) / 2;

		// Apply brake torque
		FLWheelTire->AddBrakeTorque(frontBrakeTorque);
		FRWheelTire->AddBrakeTorque(frontBrakeTorque);
		RLWheelTire->AddBrakeTorque(rearBrakeTorque);
		RRWheelTire->AddBrakeTorque(rearBrakeTorque);
		

		// Add downforce
		glm::vec3 forward = GetEntity()->GetComponent<Transform>()->GetForward();
		glm::vec3 down = -GetEntity()->GetComponent<Transform>()->GetUp();

		float forwardSpeed = glm::dot(rb->GetVelocity(), forward);

		forwardSpeed = std::max(0.0f, forwardSpeed);

		float speedRatio = forwardSpeed / referenceSpeed;
		float scale = speedRatio * speedRatio;

		glm::vec3 rearDownforce = down * (rearDownforceAt200 * scale);
		glm::vec3 frontDownforce = down * (frontDownforceAt200 * scale);

		rb->ApplyForce(rearDownforce, rearDownforcePos->GetComponent<Transform>()->GetPosition());
		rb->ApplyForce(frontDownforce, frontDownforcePos->GetComponent<Transform>()->GetPosition());

		// Add drag
		glm::vec3 velocity = rb->GetVelocity();
		float speed = glm::length(velocity);

		glm::vec3 dragDirection = -glm::normalize(velocity);
		float airDensity = 1.225f;

		float dragForceMag = 0.5f * airDensity * speed * speed * dragCoefficient * frontalArea;
		glm::vec3 dragForce = dragDirection * dragForceMag;
		rb->AddForce(dragForce);
	}

	std::string FormatTo2DP(float value)
	{
		std::ostringstream stream;
		stream << std::fixed << std::setprecision(2) << value;
		return stream.str();
	}

	enum class MenuState { Closed, Main, Controller, Graphics, Car };
	MenuState menuState = MenuState::Closed;

	void OnGUI()
	{
		auto gui = GetCore()->GetGUI();
		int width, height;
		GetCore()->GetWindow()->GetWindowSize(width, height);

		// Toggle / navigate with ESC
		if (GetKeyboard()->IsKeyDown(SDLK_ESCAPE))
		{
			if (menuState == MenuState::Closed)
			{
				GetCore()->SetTimeScale(0.f);
				menuState = MenuState::Main;
			}
			else if (menuState == MenuState::Main)
			{
				GetCore()->SetTimeScale(1.f);
				menuState = MenuState::Closed;
			}
			else
			{
				menuState = MenuState::Main; // from any submenu back to main
			}
		}

		// HUD
		gui->Image(vec2(width / 2, 25), vec2(750, 25), GetCore()->GetResources()->Load<Texture>("images/white"));

		float normalized = (currentRPM - 6000) / (maxRPM - 6000);
		float revBlend = glm::clamp(normalized, 0.0f, 1.0f);
		gui->BlendImage(vec2(width / 2, 200), vec2(750, 100),
			GetCore()->GetResources()->Load<Texture>("images/white"),
			GetCore()->GetResources()->Load<Texture>("images/senegal"),
			revBlend);

		gui->BlendImage(vec2(150, 175), vec2(200, 75),
			GetCore()->GetResources()->Load<Texture>("images/white"),
			GetCore()->GetResources()->Load<Texture>("images/green"),
			mThrottleInput);
		gui->BlendImage(vec2(150, 75), vec2(200, 75),
			GetCore()->GetResources()->Load<Texture>("images/white"),
			GetCore()->GetResources()->Load<Texture>("images/red"),
			mBrakeInput);
		gui->BlendImage(vec2(150, 20), vec2(200, 20),
			GetCore()->GetResources()->Load<Texture>("images/white"),
			GetCore()->GetResources()->Load<Texture>("images/lightBlue"),
			1 - clutchEngagement);

		if (mSteerInput > 0)
			gui->BlendImage(vec2((width / 2) - 750 / 4, 25), vec2(750 / 2, 25),
				GetCore()->GetResources()->Load<Texture>("images/black"),
				GetCore()->GetResources()->Load<Texture>("images/white"),
				1 - (mSteerInput / maxTireSteeringAngle));
		else if (mSteerInput < 0)
			gui->BlendImage(vec2((width / 2) + 750 / 4, 25), vec2((750 / 2) + 2, 25),
				GetCore()->GetResources()->Load<Texture>("images/white"),
				GetCore()->GetResources()->Load<Texture>("images/black"),
				(mSteerInput / -maxTireSteeringAngle));

		float speed = glm::dot(rb->GetVelocity(), GetEntity()->GetComponent<Transform>()->GetForward());
		gui->Text(vec2(width - 200, 100), 100, vec3(1, 1, 1), std::to_string((int)(speed * 3.6)), GetCore()->GetResources()->Load<Font>("fonts/munro"));
		gui->Text(vec2(width - 50, 60), 25, vec3(1, 1, 1), "km/h", GetCore()->GetResources()->Load<Font>("fonts/munro"));

		gui->Text(vec2(width / 2, 100), 75, vec3(1, 1, 1), std::to_string(mEngine.GetCurrentGear()), GetCore()->GetResources()->Load<Font>("fonts/munro"));
		gui->Text(vec2((width / 2) + 100, 75), 50, vec3(1, 1, 1), std::to_string((int)(currentRPM)), GetCore()->GetResources()->Load<Font>("fonts/munro"));

		// Menu overlays
		if (menuState != MenuState::Closed)
		{
			auto res = GetCore()->GetResources();
			auto white = res->Load<Texture>("images/white");
			auto dim = res->Load<Texture>("images/transparentblack");
			auto font = res->Load<Font>("fonts/munro");

			gui->Image(vec2(width / 2, height / 2), vec2(width, height), dim);

			if (menuState == MenuState::Main)
			{
				if (gui->Button(vec2(200, 150), vec2(250, 100), white) == GUI::ButtonState::Clicked)
				{
					menuState = MenuState::Closed;
					GetCore()->SetTimeScale(1.f);
				}
				gui->Text(vec2(200, 150), 50, vec3(0, 0, 0), "< Back", font);

				if (gui->Button(vec2(width / 2, height / 2 - 120), vec2(600, 150), white) == GUI::ButtonState::Clicked)
					menuState = MenuState::Controller;
				gui->Text(vec2(width / 2, height / 2 - 120), 50, vec3(0, 0, 0), "Controller Settings", font);

				if (gui->Button(vec2(width / 2, height / 2 + 40), vec2(600, 150), white) == GUI::ButtonState::Clicked)
					menuState = MenuState::Graphics;
				gui->Text(vec2(width / 2, height / 2 + 40), 50, vec3(0, 0, 0), "Graphics", font);

				if (gui->Button(vec2(width / 2, height / 2 + 200), vec2(600, 150), white) == GUI::ButtonState::Clicked)
					menuState = MenuState::Car;
				gui->Text(vec2(width / 2, height / 2 + 200), 50, vec3(0, 0, 0), "Car Settings", font);

				if (gui->Button(vec2(width - 200, 150), vec2(300, 150), white) == GUI::ButtonState::Clicked)
					GetCore()->End();
				gui->Text(vec2(width - 200, 150), 50, vec3(0, 0, 0), "CLOSE\nGAME", font);
			}
			else if (menuState == MenuState::Controller)
			{
				// Back
				if (gui->Button(vec2(200, 150), vec2(250, 100), white) == GUI::ButtonState::Clicked)
					menuState = MenuState::Main;
				gui->Text(vec2(200, 150), 50, vec3(0, 0, 0), "< Back", font);

				// Brake
				gui->Image(vec2(width / 3, height - (height / 4)), vec2(450, 200), white);
				gui->Text(vec2(width / 3, height - (height / 4)), 100, vec3(0, 0, 0), FormatTo2DP(mBrakeMaxInput), font);
				gui->Text(vec2(width / 3, (height - (height / 4)) - 75), 40, vec3(0, 0, 0), "Max brake value", font);
				if (gui->Button(vec2((width / 3) - 225 - 50, height - (height / 4)), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mBrakeMaxInput -= 0.01f;
					mBrakeMaxInput = glm::clamp(mBrakeMaxInput, 0.1f, 1.f);
				}
				gui->Text(vec2((width / 3) - 225 - 55, height - (height / 4)), 75, vec3(0, 0, 0), "<", font);
				if (gui->Button(vec2((width / 3) + 225 + 50, height - (height / 4)), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mBrakeMaxInput += 0.01f;
					mBrakeMaxInput = glm::clamp(mBrakeMaxInput, 0.1f, 1.f);
				}
				gui->Text(vec2((width / 3) + 225 + 50, height - (height / 4)), 75, vec3(0, 0, 0), ">", font);

				gui->Image(vec2(width / 3, height - (height / 4) * 2), vec2(450, 200), white);
				gui->Text(vec2(width / 3, height - (height / 4) * 2), 100, vec3(0, 0, 0), FormatTo2DP(mBrakeDeadZone), font);
				gui->Text(vec2(width / 3, (height - (height / 4) * 2) - 75), 40, vec3(0, 0, 0), "Brake deadzone", font);
				if (gui->Button(vec2((width / 3) - 225 - 50, height - (height / 4) * 2), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mBrakeDeadZone -= 0.01f;
					mBrakeDeadZone = glm::clamp(mBrakeDeadZone, 0.01f, 1.f);
				}
				gui->Text(vec2((width / 3) - 225 - 55, height - (height / 4) * 2), 75, vec3(0, 0, 0), "<", font);
				if (gui->Button(vec2((width / 3) + 225 + 50, height - (height / 4) * 2), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mBrakeDeadZone += 0.01f;
					mBrakeDeadZone = glm::clamp(mBrakeDeadZone, 0.01f, 1.f);
				}
				gui->Text(vec2((width / 3) + 225 + 50, height - (height / 4) * 2), 75, vec3(0, 0, 0), ">", font);

				// Throttle
				gui->Image(vec2((width / 3) * 2, height - (height / 4)), vec2(450, 200), white);
				gui->Text(vec2((width / 3) * 2, height - (height / 4)), 100, vec3(0, 0, 0), FormatTo2DP(mThrottleMaxInput), font);
				gui->Text(vec2((width / 3) * 2, (height - (height / 4)) - 75), 40, vec3(0, 0, 0), "Max throttle value", font);
				if (gui->Button(vec2(((width / 3) * 2) - 225 - 50, height - (height / 4)), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mThrottleMaxInput -= 0.01f;
					mThrottleMaxInput = glm::clamp(mThrottleMaxInput, 0.1f, 1.f);
				}
				gui->Text(vec2(((width / 3) * 2) - 225 - 55, height - (height / 4)), 75, vec3(0, 0, 0), "<", font);
				if (gui->Button(vec2(((width / 3) * 2) + 225 + 50, height - (height / 4)), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mThrottleMaxInput += 0.01f;
					mThrottleMaxInput = glm::clamp(mThrottleMaxInput, 0.1f, 1.f);
				}
				gui->Text(vec2(((width / 3) * 2) + 225 + 50, height - (height / 4)), 75, vec3(0, 0, 0), ">", font);

				gui->Image(vec2((width / 3) * 2, height - (height / 4) * 2), vec2(450, 200), white);
				gui->Text(vec2((width / 3) * 2, height - (height / 4) * 2), 100, vec3(0, 0, 0), FormatTo2DP(mThrottleDeadZone), font);
				gui->Text(vec2((width / 3) * 2, (height - (height / 4) * 2) - 75), 40, vec3(0, 0, 0), "Throttle deadzone", font);
				if (gui->Button(vec2(((width / 3) * 2) - 225 - 50, height - (height / 4) * 2), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mThrottleDeadZone -= 0.01f;
					mThrottleDeadZone = glm::clamp(mThrottleDeadZone, 0.01f, 1.f);
				}
				gui->Text(vec2(((width / 3) * 2) - 225 - 55, height - (height / 4) * 2), 75, vec3(0, 0, 0), "<", font);
				if (gui->Button(vec2(((width / 3) * 2) + 225 + 50, height - (height / 4) * 2), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mThrottleDeadZone += 0.01f;
					mThrottleDeadZone = glm::clamp(mThrottleDeadZone, 0.01f, 1.f);
				}
				gui->Text(vec2(((width / 3) * 2) + 225 + 50, height - (height / 4) * 2), 75, vec3(0, 0, 0), ">", font);

				// Steering
				gui->Image(vec2(width / 2, height - (height / 4) * 3), vec2(450, 200), white);
				gui->Text(vec2((width / 2), height - (height / 4) * 3), 100, vec3(0, 0, 0), FormatTo2DP(mSteerDeadzone), font);
				gui->Text(vec2((width / 2), (height - (height / 4) * 3) - 75), 40, vec3(0, 0, 0), "Steering deadzone", font);
				if (gui->Button(vec2(((width / 2)) - 225 - 50, height - (height / 4) * 3), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mSteerDeadzone -= 0.01f;
					mSteerDeadzone = glm::clamp(mSteerDeadzone, 0.01f, 1.f);
				}
				gui->Text(vec2(((width / 2)) - 225 - 55, height - (height / 4) * 3), 75, vec3(0, 0, 0), "<", font);
				if (gui->Button(vec2(((width / 2)) + 225 + 50, height - (height / 4) * 3), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					mSteerDeadzone += 0.01f;
					mSteerDeadzone = glm::clamp(mSteerDeadzone, 0.01f, 1.f);
				}
				gui->Text(vec2(((width / 2)) + 225 + 50, height - (height / 4) * 3), 75, vec3(0, 0, 0), ">", font);

				// Close game
				if (gui->Button(vec2(width - 200, 150), vec2(300, 150), white) == GUI::ButtonState::Clicked)
					GetCore()->End();
				gui->Text(vec2(width - 200, 150), 50, vec3(0, 0, 0), "CLOSE\nGAME", font);
			}
			else if (menuState == MenuState::Graphics)
			{
				if (gui->Button(vec2(200, 150), vec2(250, 100), white) == GUI::ButtonState::Clicked)
					menuState = MenuState::Main;
				gui->Text(vec2(200, 150), 50, vec3(0, 0, 0), "< Back", font);

				// VSync
				if (gui->Button(vec2(width / 2, height - (height / 2)), vec2(450, 200), white) == GUI::ButtonState::Clicked)
				{
					vsyncStatus = !vsyncStatus;
					GetCore()->GetWindow()->SetVSync(vsyncStatus);
				}
				gui->Text(vec2(width / 2, height - (height / 2)), 75, vec3(0, 0, 0), vsyncStatus ? "VSync: ON" : "VSync: OFF", font);

				if (gui->Button(vec2(width - 200, 150), vec2(300, 150), white) == GUI::ButtonState::Clicked)
					GetCore()->End();
				gui->Text(vec2(width - 200, 150), 50, vec3(0, 0, 0), "CLOSE\nGAME", font);
			}
			else if (menuState == MenuState::Car)
			{
				// Back
				if (gui->Button(vec2(200, 150), vec2(250, 100), white) == GUI::ButtonState::Clicked)
					menuState = MenuState::Main;
				gui->Text(vec2(200, 150), 50, vec3(0, 0, 0), "< Back", font);

				// Get params by reference (so edits are live)
				SuspensionParams& FL = FLWheelSuspension->GetSuspensionParams();
				SuspensionParams& FR = FRWheelSuspension->GetSuspensionParams();
				SuspensionParams& RL = RLWheelSuspension->GetSuspensionParams();
				SuspensionParams& RR = RRWheelSuspension->GetSuspensionParams();

				// Layout like your Controller page
				const float colFront = width / 3.0f;            // Front column center
				const float colRear = (width / 3.0f) * 2.0f;   // Rear column center
				const float y = height - (height / 2.0f);
				const vec2  boxSize = vec2(500, 250);

				// Front spring stiffness
				gui->Image(vec2(colFront, y), boxSize, white);
				gui->Text(vec2(colFront, y - 75.0f), 40, vec3(0, 0, 0), "Front spring stiffness", font);
                gui->Text(vec2(colFront, y), 100, vec3(0, 0, 0), std::to_string((int)FL.stiffness), font);

				if (gui->Button(vec2(colFront - (boxSize.x / 2) - 50.0f, y), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					float nv = glm::clamp(FL.stiffness - 5000.0f, 100000.0f, 200000.0f);
					FL.stiffness = FR.stiffness = nv;
				}
				gui->Text(vec2(colFront - (boxSize.x / 2) - 55.0f, y), 75, vec3(0, 0, 0), "<", font);

				if (gui->Button(vec2(colFront + (boxSize.x / 2) + 50.0f, y), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					float nv = glm::clamp(FL.stiffness + 5000.0f, 100000.0f, 200000.0f);
					FL.stiffness = FR.stiffness = nv;
				}
				gui->Text(vec2(colFront + (boxSize.x / 2) + 50.0f, y), 75, vec3(0, 0, 0), ">", font);

				// Rear spring stiffness
				gui->Image(vec2(colRear, y), boxSize, white);
				gui->Text(vec2(colRear, y - 75.0f), 40, vec3(0, 0, 0), "Rear spring stiffness", font);
				gui->Text(vec2(colRear, y), 100, vec3(0, 0, 0), std::to_string((int)RL.stiffness), font);

				if (gui->Button(vec2(colRear - (boxSize.x / 2) - 50.0f, y), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					float nv = glm::clamp(RL.stiffness - 5000.0f, 100000.0f, 200000.0f);
					RL.stiffness = RR.stiffness = nv;
				}
				gui->Text(vec2(colRear - (boxSize.x / 2) - 55.0f, y), 75, vec3(0, 0, 0), "<", font);

				if (gui->Button(vec2(colRear + (boxSize.x / 2) + 50.0f, y), vec2(50, 100), white) == GUI::ButtonState::Clicked)
				{
					float nv = glm::clamp(RL.stiffness + 5000.0f, 100000.0f, 200000.0f);
					RL.stiffness = RR.stiffness = nv;
				}
				gui->Text(vec2(colRear + (boxSize.x / 2) + 50.0f, y), 75, vec3(0, 0, 0), ">", font);

				// Close game
				if (gui->Button(vec2(width - 200, 150), vec2(300, 150), white) == GUI::ButtonState::Clicked)
					GetCore()->End();
				gui->Text(vec2(width - 200, 150), 50, vec3(0, 0, 0), "CLOSE\nGAME", font);
			}
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

		if (GetKeyboard()->IsKeyDown(SDLK_b))
		{
			GetCore()->SetTimeScale(0.1);
		}
		else if (GetKeyboard()->IsKeyDown(SDLK_n))
		{
			GetCore()->SetTimeScale(1);
		}
		else if (GetKeyboard()->IsKeyDown(SDLK_m))
		{
			GetCore()->SetTimeScale(0);
		}
	}

	float mfpsTimer = 0.f;
	int currentFPS = 0;

	void OnGUI()
	{
		mfpsTimer += GetCore()->GetLastFrameTime();

		if (mfpsTimer > 0.5f)
		{
			mfpsTimer = 0.f;
			currentFPS = (int)(1.0f / GetCore()->GetLastFrameTime());
		}

		int width, height;
		GetCore()->GetWindow()->GetWindowSize(width, height);

		GetGUI()->Text(vec2(60, height - 20), 40, vec3(0, 1, 0), std::to_string(currentFPS), GetCore()->GetResources()->Load<Font>("fonts/munro"));
	}
};

struct printPosition : Component
{
	void OnTick()
	{
		vec3 pos = GetEntity()->GetComponent<Transform>()->GetLocalPosition();
		std::cout << GetEntity()->GetTag() << " position: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
	}
};

#undef main
int main()
{
	std::shared_ptr<Core> core = Core::Initialize(ivec2(1920, 1080));
	core->SetLoadingScreen(core->GetResources()->Load<Texture>("images/loading"));
	core->SetTimeScale(1.f);

	// Scope to ensure the entities aren't being held in main if they're destroyed
	{

		TireParams frontTyreParams{};
		frontTyreParams.brushLongStiffCoeff = 80;
		frontTyreParams.brushLatStiffCoeff = 110;

		frontTyreParams.peakFrictionCoefficient = 1.6f;
		frontTyreParams.tireRadius = 0.34f;
		frontTyreParams.wheelMass = 25.f;
		frontTyreParams.rollingResistance = 0.015f;

		TireParams rearTyreParams{};
		rearTyreParams.brushLongStiffCoeff = 80;
		rearTyreParams.brushLatStiffCoeff = 110;

		rearTyreParams.peakFrictionCoefficient = 1.6f;
		rearTyreParams.tireRadius = 0.34f;
		rearTyreParams.wheelMass = 25.f;
		rearTyreParams.rollingResistance = 0.015f;


		SuspensionParams frontSuspensionParams{};
		frontSuspensionParams.stiffness = 140000;
		frontSuspensionParams.bumpDampLowSpeed = 6000;
		frontSuspensionParams.bumpDampHighSpeed = 11000;
		frontSuspensionParams.reboundDampLowSpeed = 5500;
		frontSuspensionParams.reboundDampHighSpeed = 8000;
		frontSuspensionParams.bumpStopStiffness = 160000;
		frontSuspensionParams.bumpStopRange = 0.015f;
		frontSuspensionParams.antiRollBarStiffness = 35000;
		frontSuspensionParams.restLength = 0.42f;
		frontSuspensionParams.rideHeight = 0.052f;

		SuspensionParams rearSuspensionParams{};
		rearSuspensionParams.stiffness = 105000;
		rearSuspensionParams.bumpDampLowSpeed = 5500;
		rearSuspensionParams.bumpDampHighSpeed = 9000;
		rearSuspensionParams.reboundDampLowSpeed = 5000;
		rearSuspensionParams.reboundDampHighSpeed = 10000;
		rearSuspensionParams.bumpStopStiffness = 160000;
		rearSuspensionParams.bumpStopRange = 0.015f;
		rearSuspensionParams.antiRollBarStiffness = 32000;
		rearSuspensionParams.restLength = 0.44f;
		rearSuspensionParams.rideHeight = 0.072f;


		core->GetSkybox()->SetTexture(core->GetResources()->Load<SkyboxTexture>("skyboxes/sky"));

		core->GetLightManager()->SetDirectionalLightDirection(glm::vec3(0.5f, -1.0f, 0.5f));
		core->GetLightManager()->SetAmbient(vec3(0.3f));
		core->GetLightManager()->SetupDefault3Cascades();

		// Cameras
		std::shared_ptr<Entity> freeCamEntity = core->AddEntity();
		std::shared_ptr<Camera> freeCam = freeCamEntity->AddComponent<Camera>();
		freeCam->SetPriority(10);
		freeCamEntity->GetComponent<Transform>()->SetPosition(vec3(647.479, -65.4695, -252.504));
		freeCamEntity->GetComponent<Transform>()->SetRotation(vec3(0, 180, 0));
		freeCamEntity->AddComponent<freelookCamController>();

		std::shared_ptr<Entity> cockpitCamEntity = core->AddEntity();
		std::shared_ptr<Camera> cockpitCam = cockpitCamEntity->AddComponent<Camera>();
		cockpitCam->SetPriority(10);
		cockpitCamEntity->GetComponent<Transform>()->SetPosition(vec3(0.390911, 0.611829, -0.526169));
		cockpitCamEntity->GetComponent<Transform>()->SetRotation(vec3(0, 180, 0));
		cockpitCamEntity->AddComponent<CameraController>();

		std::shared_ptr<Entity> bonnetCamEntity = core->AddEntity();
		std::shared_ptr<Camera> bonnetCam = bonnetCamEntity->AddComponent<Camera>();
		bonnetCam->SetPriority(1);
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

		// Track, Imola
		std::shared_ptr<Entity> track = core->AddEntity();
		track->SetTag("track");
		track->GetComponent<Transform>()->SetPosition(vec3(0, 0, 0));
		track->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		std::shared_ptr<ModelRenderer> trackMR = track->AddComponent<ModelRenderer>();
		trackMR->SetModel(core->GetResources()->Load<Model>("models/Imola/Imola.glb"));
		trackMR->SetSpecularStrength(0.f);
		trackMR->SetPreBakeShadows(true);
		std::shared_ptr<ModelCollider> trackCollider = track->AddComponent<ModelCollider>();
		trackCollider->SetModel(core->GetResources()->Load<Model>("models/Imola/Imola.glb"));
		trackCollider->SetDebugVisual(false);

		// Start/finish line
		std::shared_ptr<Entity> startFinishLine = core->AddEntity();
		startFinishLine->SetTag("startFinishLine");
		startFinishLine->GetComponent<Transform>()->SetPosition(vec3(204.75, -84.9107, -384.765));
		startFinishLine->GetComponent<Transform>()->SetRotation(vec3(0, 1, 0));
		std::shared_ptr<BoxCollider> startFinishLineCollider = startFinishLine->AddComponent<BoxCollider>();
		startFinishLineCollider->SetSize(vec3(0.1, 6, 60));
		startFinishLineCollider->IsTrigger(true);
		std::shared_ptr <StartFinishLine> startFinishLineComponent = startFinishLine->AddComponent<StartFinishLine>();

		// Car Body
		std::shared_ptr<Entity> carBody = core->AddEntity();
		carBody->SetTag("carBody");
		carBody->GetComponent<Transform>()->SetPosition(vec3(647.479, -65.4695, -252.504));
		carBody->GetComponent<Transform>()->SetRotation(vec3(177.438, 48.71, -179.937));
		carBody->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> mercedesMR = carBody->AddComponent<ModelRenderer>();
		mercedesMR->SetRotationOffset(vec3(0, 180, 0));
		mercedesMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/RawCar.glb"));
		mercedesMR->SetMetallicness(1.0f);
		mercedesMR->SetRoughness(0.5f);
		std::shared_ptr<BoxCollider> carBodyCollider = carBody->AddComponent<BoxCollider>();
		carBodyCollider->SetSize(vec3(1.97, 0.9, 4.52));
		carBodyCollider->SetPositionOffset(vec3(0, 0.37, 0.22));
		std::shared_ptr<Rigidbody> carBodyRB = carBody->AddComponent<Rigidbody>();
		carBodyRB->SetMass(1230);

		std::shared_ptr<Entity> steeringWheel = core->AddEntity();
		steeringWheel->SetTag("steeringWheel");
		steeringWheel->GetComponent<Transform>()->SetPosition(vec3(0.4007, 0.3745, 0.174));
		steeringWheel->GetComponent<Transform>()->SetRotation(vec3(0, 180, 0));
		std::shared_ptr<ModelRenderer> steeringWheelMR = steeringWheel->AddComponent<ModelRenderer>();
		steeringWheelMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/SteeringWheel.glb"));
		steeringWheel->GetComponent<Transform>()->SetParent(carBody);


		cockpitCamEntity->GetComponent<Transform>()->SetParent(carBody);
		bonnetCamEntity->GetComponent<Transform>()->SetParent(carBody);
		chaseCamEntity->GetComponent<Transform>()->SetParent(carBody);
		wheelCamEntity->GetComponent<Transform>()->SetParent(carBody);

		//freeCamEntity->GetComponent<Transform>()->SetParent(carBody);

		startFinishLineComponent->car = carBody;

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
		FLWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(0.856, 0.38, 1.6));
		FLWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.01, 0.01, 0.01));
		FLWheelAnchor->GetComponent<Transform>()->SetParent(carBody);

		std::shared_ptr<Entity> FRWheelAnchor = core->AddEntity();
		FRWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(-0.856, 0.38, 1.6));
		FRWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.01, 0.01, 0.01));
		FRWheelAnchor->GetComponent<Transform>()->SetParent(carBody);

		std::shared_ptr<Entity> RLWheelAnchor = core->AddEntity();
		RLWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(0.863, 0.38, -1.027));
		RLWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.01, 0.01, 0.01));
		RLWheelAnchor->GetComponent<Transform>()->SetParent(carBody);

		std::shared_ptr<Entity> RRWheelAnchor = core->AddEntity();
		RRWheelAnchor->GetComponent<Transform>()->SetPosition(vec3(-0.863, 0.38, -1.027));
		RRWheelAnchor->GetComponent<Transform>()->SetScale(vec3(0.01, 0.01, 0.01));
		RRWheelAnchor->GetComponent<Transform>()->SetParent(carBody);


		// Front Left Wheel
		std::shared_ptr<Entity> FLWheel = core->AddEntity();
		FLWheel->SetTag("FLwheel");
		FLWheel->GetComponent<Transform>()->SetPosition(vec3(0.8767, 0.793 - 0.45, -14.3998));
		FLWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		FLWheel->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> FLWheelMR = FLWheel->AddComponent<ModelRenderer>();
		FLWheelMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/WheelNoBrake.glb"));
		FLWheelMR->SetRotationOffset(vec3(0, -90, 0));
		std::shared_ptr<Suspension> FLWheelSuspension = FLWheel->AddComponent<Suspension>();
		FLWheelSuspension->SetWheel(FLWheel);
		FLWheelSuspension->SetCarBody(carBody);
		FLWheelSuspension->SetAnchorPoint(FLWheelAnchor);
		FLWheelSuspension->SetSuspensionParams(frontSuspensionParams);
		std::shared_ptr<Tire> FLWheelTire = FLWheel->AddComponent<Tire>();
		FLWheelTire->SetCarBody(carBody);
		FLWheelTire->SetAnchorPoint(FLWheelAnchor);
		FLWheelTire->SetTireParams(frontTyreParams);
		FLWheelTire->SetInitialRotationOffset(vec3(0, -90, 0));

		std::shared_ptr<Entity> FLWheelBrake = core->AddEntity();
		FLWheelBrake->SetTag("FLwheelBrake");
		std::shared_ptr<ModelRenderer> FLBrakeMR = FLWheelBrake->AddComponent<ModelRenderer>();
		FLBrakeMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/FrontBrake.glb"));
		FLWheelBrake->GetComponent<Transform>()->SetParent(FLWheel);
		FLWheelBrake->GetComponent<Transform>()->SetRotation(vec3(0, 90, 0));

		// Front Right Wheel
		std::shared_ptr<Entity> FRWheel = core->AddEntity();
		FRWheel->SetTag("FRwheel");
		FRWheel->GetComponent<Transform>()->SetPosition(vec3(-0.8767, 0.793 - 0.45, -14.3998));
		FRWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		FRWheel->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> FRWheelMR = FRWheel->AddComponent<ModelRenderer>();
		FRWheelMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/WheelNoBrake.glb"));
		FRWheelMR->SetRotationOffset(vec3(0, 90, 0));
		std::shared_ptr<Suspension> FRWheelSuspension = FRWheel->AddComponent<Suspension>();
		FRWheelSuspension->SetWheel(FRWheel);
		FRWheelSuspension->SetCarBody(carBody);
		FRWheelSuspension->SetAnchorPoint(FRWheelAnchor);
		FRWheelSuspension->SetSuspensionParams(frontSuspensionParams);
		std::shared_ptr<Tire> FRWheelTire = FRWheel->AddComponent<Tire>();
		FRWheelTire->SetCarBody(carBody);
		FRWheelTire->SetAnchorPoint(FRWheelAnchor);
		FRWheelTire->SetTireParams(frontTyreParams);
		FRWheelTire->SetInitialRotationOffset(vec3(0, 90, 0));

		std::shared_ptr<Entity> FRWheelBrake = core->AddEntity();
		FRWheelBrake->SetTag("FRwheelBrake");
		std::shared_ptr<ModelRenderer> FRBrakeMR = FRWheelBrake->AddComponent<ModelRenderer>();
		FRBrakeMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/FrontBrake.glb"));
		FRWheelBrake->GetComponent<Transform>()->SetParent(FRWheel);
		FRWheelBrake->GetComponent<Transform>()->SetRotation(vec3(0, 90, 0));

		// Rear Left Wheel
		std::shared_ptr<Entity> RLWheel = core->AddEntity();
		RLWheel->SetTag("RLwheel");
		RLWheel->GetComponent<Transform>()->SetPosition(vec3(0.8767, 0.8003 - 0.45, -17.025));
		RLWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		RLWheel->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> RLWheelMR = RLWheel->AddComponent<ModelRenderer>();
		RLWheelMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/WheelNoBrake.glb"));
		RLWheelMR->SetRotationOffset(vec3(0, -90, 0));
		std::shared_ptr<Suspension> RLWheelSuspension = RLWheel->AddComponent<Suspension>();
		RLWheelSuspension->SetWheel(RLWheel);
		RLWheelSuspension->SetCarBody(carBody);
		RLWheelSuspension->SetAnchorPoint(RLWheelAnchor);
		RLWheelSuspension->SetSuspensionParams(rearSuspensionParams);
		std::shared_ptr<Tire> RLWheelTire = RLWheel->AddComponent<Tire>();
		RLWheelTire->SetCarBody(carBody);
		RLWheelTire->SetAnchorPoint(RLWheelAnchor);
		RLWheelTire->SetTireParams(rearTyreParams);
		RLWheelTire->SetInitialRotationOffset(vec3(0, -90, 0));

		std::shared_ptr<Entity> RLWheelBrake = core->AddEntity();
		RLWheelBrake->SetTag("RLwheelBrake");
		std::shared_ptr<ModelRenderer> RLBrakeMR = RLWheelBrake->AddComponent<ModelRenderer>();
		RLBrakeMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/RearBrake.glb"));
		RLWheelBrake->GetComponent<Transform>()->SetParent(RLWheel);
		RLWheelBrake->GetComponent<Transform>()->SetRotation(vec3(0, -90, 0));

		// Rear Right Wheel
		std::shared_ptr<Entity> RRWheel = core->AddEntity();
		RRWheel->SetTag("RRwheel");
		RRWheel->GetComponent<Transform>()->SetPosition(vec3(-0.8767, 0.8003 - 0.45, -17.025));
		RRWheel->GetComponent<Transform>()->SetRotation(vec3(0, 0, 0));
		RRWheel->GetComponent<Transform>()->SetScale(vec3(1, 1, 1));
		std::shared_ptr<ModelRenderer> RRWheelMR = RRWheel->AddComponent<ModelRenderer>();
		RRWheelMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/WheelNoBrake.glb"));
		RRWheelMR->SetRotationOffset(vec3(0, 90, 0));
		std::shared_ptr<Suspension> RRWheelSuspension = RRWheel->AddComponent<Suspension>();
		RRWheelSuspension->SetWheel(RRWheel);
		RRWheelSuspension->SetCarBody(carBody);
		RRWheelSuspension->SetAnchorPoint(RRWheelAnchor);
		RRWheelSuspension->SetSuspensionParams(rearSuspensionParams);
		std::shared_ptr<Tire> RRWheelTire = RRWheel->AddComponent<Tire>();
		RRWheelTire->SetCarBody(carBody);
		RRWheelTire->SetAnchorPoint(RRWheelAnchor);
		RRWheelTire->SetTireParams(rearTyreParams);
		RRWheelTire->SetInitialRotationOffset(vec3(0, 90, 0));

		std::shared_ptr<Entity> RRWheelBrake = core->AddEntity();
		RRWheelBrake->SetTag("RRwheelBrake");
		std::shared_ptr<ModelRenderer> RRBrakeMR = RRWheelBrake->AddComponent<ModelRenderer>();
		RRBrakeMR->SetModel(core->GetResources()->Load<Model>("models/Mercedes/RearBrake.glb"));
		RRWheelBrake->GetComponent<Transform>()->SetParent(RRWheel);
		RRWheelBrake->GetComponent<Transform>()->SetRotation(vec3(0, -90, 0));

		FLWheelSuspension->SetOppositeAxelSuspension(FRWheelSuspension);
		FRWheelSuspension->SetOppositeAxelSuspension(FLWheelSuspension);
		RLWheelSuspension->SetOppositeAxelSuspension(RRWheelSuspension);
		RRWheelSuspension->SetOppositeAxelSuspension(RLWheelSuspension);

		std::shared_ptr<CarController> carController = carBody->AddComponent<CarController>();
		carController->rb = carBodyRB;
		carController->FLWheelSuspension = FLWheelSuspension;
		carController->FRWheelSuspension = FRWheelSuspension;
		carController->RLWheelSuspension = RLWheelSuspension;
		carController->RRWheelSuspension = RRWheelSuspension;
		carController->FLWheelTire = FLWheelTire;
		carController->FRWheelTire = FRWheelTire;
		carController->RLWheelTire = RLWheelTire;
		carController->RRWheelTire = RRWheelTire;
		carController->frontDownforcePos = frontDownForcePos;
		carController->rearDownforcePos = rearDownForcePos;
		carController->steeringWheel = steeringWheel;

	}

	core->Run();
}