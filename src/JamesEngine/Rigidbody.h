#pragma once

#include "Component.h"
#include <iostream>

namespace JamesEngine
{

	class Rigidbody : public Component
	{
	public:
		void OnInitialize();
		void OnTick();

		void AddForce(glm::vec3 _force) { mForce += _force; }
		void AddTorque(glm::vec3 _torque) { mTorque += _torque; }
		void ClearForces() { mForce = glm::vec3(0); mTorque = glm::vec3(0); }

		void SetMass(float _mass) { mMass = _mass; }
		float GetMass() { return mMass; }

		void SetForce(glm::vec3 _force) { mForce = _force; }
		glm::vec3 GetForce() { return mForce; }

		void SetTorque(glm::vec3 _torque) { mTorque = _torque; }
		glm::vec3 GetTorque() { return mTorque; }

		void SetVelocity(glm::vec3 _velocity) { mVelocity = _velocity; }
		glm::vec3 GetVelocity() { return mVelocity; }

		void SetAcceleration(glm::vec3 _acceleration) { mAcceleration = _acceleration; }
		glm::vec3 GetAcceleration() { return mAcceleration; }

		

	private:

		void UpdateInertiaTensor();
		void ComputeInverseInertiaTensor();
		void CalculateEulerAngles();

		void Euler();
		//void Verlet();

		glm::vec3 mForce = glm::vec3(0);
		glm::vec3 mVelocity = glm::vec3(0);
		glm::vec3 mAcceleration = glm::vec3(0, -9.81f, 0);
		glm::vec3 mPreviousPosition = glm::vec3(0);

		float mMass = 1;

		glm::vec3 mTorque = glm::vec3{ 0 };
		glm::vec3 mAngularVelocity = glm::vec3{ 0 };
		glm::vec3 mAngularMomentum = glm::vec3{ 0 };
		glm::mat3 mInertiaTensorInverse = glm::mat3{ 0 };
		glm::mat3 mBodyInertiaTensorInverse = glm::mat3{ 0 };
		glm::mat3 mR = glm::mat3(	1.0f, 0.0f, 0.0f,
									0.0f, 1.0f, 0.0f,
									0.0f, 0.0f, 1.0f);

		float globalDamping = 125;
	};

}