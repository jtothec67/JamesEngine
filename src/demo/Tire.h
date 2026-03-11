#pragma once

#include "JamesEngine/JamesEngine.h"

namespace JamesEngine
{

	struct TireParams
	{
		float peakFrictionCoefficient;

		float peakFrictionCoeffLong;
		float peakFrictionCoeffLat;

		float slidingFrictionFactorLong;
		float slidingFrictionFactorLat;

		float slidingFrictionFalloffExponentLong;
		float slidingFrictionFalloffExponentLat;

		float longStiffCoeff;
		float latStiffCoeff;

		float loadSensitivityRef;
		float longStiffExp;
		float latStiffExp;

		float contactHalfLengthY; // Lateral width, stays constant
		float maxContactHalfLengthX; // Longitudinal length, changes with load
		float refMaxLoadContactHalfLengthX; // Contact patch length at reference max load

		float tireRadius;
		float wheelMass;
		float rollingResistance;
	};

	class Rigidbody;
	class Suspension;
	class AudioSource;

	class Tire : public Component
	{
	public:
		void OnAlive();
		void OnFixedTick();
		void OnTick();

		void AddDriveTorque(float _torque) { mDriveTorque += _torque; }
		void AddBrakeTorque(float _torque) { mBrakeTorque += _torque; }

		void SetCarBody(std::shared_ptr<Entity> _carBody) { mCarBody = _carBody; }
		void SetAnchorPoint(std::shared_ptr<Entity> _anchorPoint) { mAnchorPoint = _anchorPoint; }

		void SetTireParams(const TireParams& _tireParams) { mTireParams = _tireParams; }

		void SetInitialRotationOffset(const glm::vec3& _offset) { mInitialRotationOffset = _offset; }

		void SetAngularVelocity(float _angularVelocity) { mWheelAngularVelocity = _angularVelocity; }
		float GetAngularVelocity() const { return mWheelAngularVelocity; }

		float GetInertia() const { return mInertia; }

		void IsSliding(bool _isSliding) { mIsSliding = _isSliding; }

		float GetGripUsage() { return mGripUsage; }

		float GetSlidingAmount();

	private:
		glm::vec2 BrushTireModel(float Vx, float Vy, float omega, float Fz);

		std::shared_ptr<Entity> mCarBody;
		std::shared_ptr<Entity> mAnchorPoint;

		std::shared_ptr<Rigidbody> mWheelRb;
		std::shared_ptr<Rigidbody> mCarRb;

		std::shared_ptr<Suspension> mSuspension;

		std::shared_ptr<AudioSource> mAudioSource;

		TireParams mTireParams;

		float mDriveTorque = 0.f;
		float mBrakeTorque = 0.f;

		float mWheelAngularVelocity = 0.f;

		float mInertia = 1.f; // Set based on wheel mass and radius at the first frame

		float mWheelRotation = 0.f;

		bool mIsSliding = false;

		glm::vec3 mInitialRotationOffset = glm::vec3(0.f, 0.f, 0.f);

		float mGripUsage = 0.f; // How much grip is being used. 1 is on limit of grip, over 1 is above limit of grip

		float mLastSlipRatio = 0.f;
		float mLastSlipAngle = 0.f;

#ifdef JAMES_DEBUG
		struct TireGraphInfo
		{
			const float slipAngStart = glm::radians(-20.0f);
			const float slipAngEnd = glm::radians(20.0f);
			const float slipAngStep = glm::radians(0.5f);

			const float maxDisplayFy = 15000.f;
			std::vector<float> fyCurve;
			float curSlipAngle = 0;
			float curFy = 0;

			const float slipRatioStart = -1.0f;
			const float slipRatioEnd = 1.0f;
			const float slipRatioStep = 0.01f;

			const float maxDisplayFx = 15000.f;
			std::vector<float> fxCurve;
			float curSlipRatio = 0;
			float curFx = 0;
		};

		TireGraphInfo mTireGraphInfo;
#endif
	};

}