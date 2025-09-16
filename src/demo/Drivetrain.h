#pragma once

#include "JamesEngine/JamesEngine.h"

namespace JamesEngine
{

	class Drivetrain
	{
	public:
		glm::vec2 SplitTorque(float _torque, glm::vec2 _angularVelocityLR);

		glm::vec2 ApplyOpenDiffKinematics(glm::vec2 _angularVelocityLR, glm::vec2 _inertiaLR, float _carrierAngularVelocity);

		void SetPreLoadNM(float _preload) { mPreloadNM = _preload; }
		void SetKPower(float _k) { mKPower = _k; }
		void SetKCoast(float _k) { mKCoast = _k; }
		void SetViscousDamping(float _c) { mcVisc = _c; }

	private:
		float mPreloadNM = 100.0f;
		float mKPower = 0.25f;
		float mKCoast = 0.15f;
		float mcVisc = 10.0f;
	};

}