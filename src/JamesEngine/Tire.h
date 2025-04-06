#pragma once

#include "Component.h"

namespace JamesEngine
{

	struct TireParams
	{
		float longitudinalStiffness;
		float lateralStiffness;
		float peakFrictionCoefficient;
		float tireRadius;
	};

	class Rigidbody;

	class Tire : public Component
	{
	public:
		void OnTick();

		void ApplyDriveTorque(float _torque);
		void ApplyBrakeTorque(float _torque);

		void SetCarBody(std::shared_ptr<Entity> _carBody) { mCarBody = _carBody; }
		void SetAnchorPoint(std::shared_ptr<Entity> _anchorPoint) { mAnchorPoint = _anchorPoint; }

		void SetTireParams(const TireParams& _tireParams) { mTireParams = _tireParams; }

	private:
		std::shared_ptr<Entity> mCarBody;
		std::shared_ptr<Entity> mAnchorPoint;

		std::shared_ptr<Rigidbody> mWheelRb;
		std::shared_ptr<Rigidbody> mCarRb;

		TireParams mTireParams;
	};

}