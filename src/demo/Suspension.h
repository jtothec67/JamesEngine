#pragma once

#include "JamesEngine/JamesEngine.h"

#ifdef _DEBUG
#include "Renderer/Shader.h"
#endif


namespace JamesEngine
{
	class Rigidbody;

	class Suspension : public Component
	{
	public:
#ifdef _DEBUG
		void OnGUI();
#endif

		void OnAlive();
		void OnEarlyFixedTick();
		void OnFixedTick();
		void OnLateFixedTick();
		void OnTick();

		void SetWheel(std::shared_ptr<Entity> _wheel) { mWheel = _wheel; }
		void SetCarBody(std::shared_ptr<Entity> _carBody) { mCarBody = _carBody; }
		void SetAnchorPoint(std::shared_ptr<Entity> _anchorPoint) { mAnchorPoint = _anchorPoint; }
		void SetOppositeAxelSuspension(std::shared_ptr<Suspension> _suspension) { mOppositeAxelSuspension = _suspension; }

		void SetStiffness(float _stiffness) { mStiffness = _stiffness; }

		void SetDampingThreshold(float _dampingThreshold) { mDampingThreshold = _dampingThreshold; }
		void SetReboundDampHigh(float _reboundDampHighSpeed) { mReboundDampHighSpeed = _reboundDampHighSpeed; }
		void SetReboundDampLow(float _reboundDampLowSpeed) { mReboundDampLowSpeed = _reboundDampLowSpeed; }
		void SetBumpDampHigh(float _bumpDampHighSpeed) { mBumpDampHighSpeed = _bumpDampHighSpeed; }
		void SetBumpDampLow(float _bumpDampLowSpeed) { mBumpDampLowSpeed = _bumpDampLowSpeed; }

		void SetBumpStopStiffness(float _bumpStopStiffness) { mBumpStopStiffness = _bumpStopStiffness; }
		void SetBumpStopRange(float _bumpStopRange) { mBumpStopRange = _bumpStopRange; }

		float SetAntiRollBarStiffness(float _antiRollBarStiffness) { mAntiRollBarStiffness = _antiRollBarStiffness; return mAntiRollBarStiffness; }

		bool GetCollision() { return mGroundContact; }

		void SetSteeringAngle(float _steeringAngle) { mSteeringAngle = _steeringAngle; }

		void SetTireRadius(float _wheelRadius) { mTireRadius = _wheelRadius; }

		void SetRestLength(float _restLength) { mRestLength = _restLength; }

		void SetRideHeight(float _rideHeight) { mRideHeight = _rideHeight; }

		void SetDebugVisual(bool _value) { mDebugVisual = _value; }

		glm::vec3 GetContactPoint() { return mContactPoint; }

		float GetForce() { return mForce; }

		float GetDisplacement() { return mDisplacement; }

		void SetSurfaceNormal(glm::vec3 _surfaceNormal) { mSurfaceNormal = _surfaceNormal; }
		glm::vec3 GetSurfaceNormal() { return mSurfaceNormal; }

	private:
		std::shared_ptr<Entity> mWheel;
		std::shared_ptr<Entity> mCarBody;
		std::shared_ptr<Entity> mAnchorPoint;
		std::shared_ptr<Suspension> mOppositeAxelSuspension;

		std::shared_ptr<Rigidbody> mCarRb;

		bool mGroundContact = false;
		glm::vec3 mContactPoint{ 0 };
		glm::vec3 mSurfaceNormal = glm::vec3(0, 1, 0);
		float mForce = 0.0f;
		float mDisplacement = 0.0f;
		float mCurrentLength = 0.0f;

		float mStiffness = 500;

		float mDampingThreshold = 0.25f; // Threshold for high-speed damping, m/s

		float mReboundDampHighSpeed = 100;
		float mReboundDampLowSpeed = 100;
		float mBumpDampHighSpeed = 100;
		float mBumpDampLowSpeed = 100;

		float mBumpStopStiffness = 1000.0f;
		float mBumpStopRange = 0.02f;

		float mAntiRollBarStiffness = 1000.0f;

		float mRestLength = 0.45f;
		float mRideHeight = 0.07f;
		float mTireRadius = 0.34f;

		float mSteeringAngle = 0.0f;

		bool mDebugVisual = true;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../assets/shaders/OutlineShader.vert", "../assets/shaders/OutlineShader.frag");
		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/spring.obj");
#endif
	};

}