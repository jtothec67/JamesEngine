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

		void SetAngularMomentum(glm::vec3 _angularMomentum) { mAngularMomentum = _angularMomentum; }
		glm::vec3 GetAngularMomentum() { return mAngularMomentum; }

		void SetAngularVelocity(glm::vec3 _angularVelocity) { mAngularVelocity = _angularVelocity; }
		glm::vec3 GetAngularVelocity() { return mAngularVelocity; }

		glm::vec3 mCollisionPoint = glm::vec3(0);

		void LockRotation(bool _lock) { mLockRotation = _lock; }

	private:
		void ApplyImpulseResponse(std::shared_ptr<Rigidbody> _other, glm::vec3 _normal, glm::vec3 _collisionPoint);
		void ApplyImpulseResponseStatic(glm::vec3 _normal, glm::vec3 _collisionPoint);
		glm::vec3 FrictionForce(glm::vec3 _relativeVelocity, glm::vec3 _contactNormal, glm::vec3 _forceNormal, float mu);
		glm::vec3 ComputeTorque(glm::vec3 torque_arm, glm::vec3 contact_force);

		void UpdateInertiaTensor();
		void ComputeInverseInertiaTensor();
		void CalculateAngles();

		void Euler();
		void Verlet();

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

		float globalDamping = 64;

		bool mLockRotation = false;
	};

}