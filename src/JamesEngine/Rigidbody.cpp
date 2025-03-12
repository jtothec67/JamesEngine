#include "Rigidbody.h"

#include "Entity.h"
#include "Core.h"
#include "Component.h"
#include "Transform.h"
#include "Collider.h"
#include "BoxCollider.h"

#include <vector>
#include <iostream>

namespace JamesEngine
{

	void Rigidbody::OnInitialize()
	{
		UpdateInertiaTensor();
	}

	void Rigidbody::OnTick()
	{
		// Step 1: Compute each of the forces acting on the object (only gravity by default)
		glm::vec3 force = mMass * mAcceleration;
		AddForce(force);

		// Step 2: Compute collisions
		// Get all colliders in the scene
		std::vector<std::shared_ptr<Collider>> colliders;
		GetEntity()->GetCore()->FindComponents(colliders);

		// Iterate through all colliders to see if we're colliding with any
		for (auto& otherCollider : colliders)
		{
			// Skip if it is ourself
			if (otherCollider->GetTransform() == GetTransform())
				continue;

			std::shared_ptr<Collider> collider = GetEntity()->GetComponent<Collider>();

			glm::vec3 collisionPoint;
			glm::vec3 collisionNormal;
			float penetrationDepth;

			// Check if colliding
			if (otherCollider->IsColliding(collider, collisionPoint, collisionNormal, penetrationDepth))
			{
				collisionNormal = glm::normalize(collisionNormal);

				// Call OnCOllision for all components on both entities
				for (size_t ci = 0; ci < GetEntity()->mComponents.size(); ci++)
				{
					GetEntity()->mComponents.at(ci)->OnCollision(otherCollider->GetEntity());
				}

				for (size_t ci = 0; ci < otherCollider->GetEntity()->mComponents.size(); ci++)
				{
					otherCollider->GetEntity()->mComponents.at(ci)->OnCollision(GetEntity());
				}

				std::shared_ptr<Rigidbody> otherRigidbody = otherCollider->GetEntity()->GetComponent<Rigidbody>();

				// Step 3: Resolve penetration and collision

				// Two rigidbodies collided
				if (otherRigidbody)
				{
					ApplyImpulseResponse(otherRigidbody, collisionNormal, collisionPoint);

					float totalInverseMass = (1.0f / mMass) + (1.0f / otherRigidbody->GetMass());

					Move(penetrationDepth * collisionNormal * (1.0f / mMass) / totalInverseMass);
					
					otherRigidbody->Move(-penetrationDepth * collisionNormal * (1.0f / otherRigidbody->GetMass()) / totalInverseMass);
				}
				else // We are rigidbody, other isn't
				{
					ApplyImpulseResponseStatic(collisionNormal, collisionPoint);

					Move(penetrationDepth * collisionNormal);
				}
				
			}
		}

		// Step 5: Integration
		Euler();

		// Setp 6: Convert to euler angles
		CalculateEulerAngles();

		// Step 7: Clear forces
		ClearForces();
	}

	void Rigidbody::ApplyImpulseResponse(std::shared_ptr<Rigidbody> _other, glm::vec3 _normal, glm::vec3 _collisionPoint)
	{
		glm::vec3 velocityA = GetVelocity();
		glm::vec3 velocityB = _other->GetVelocity();
		glm::vec3 relativeVelocity = velocityA - velocityB;

		float elasticity = 0.6f * 0.5f;
		float J_numerator = -(1.0f + elasticity) * glm::dot(relativeVelocity, _normal);
		float totalInverseMass = (1.0f / GetMass()) + (1.0f / _other->GetMass());
		float J = J_numerator / (totalInverseMass);

		glm::vec3 collisionImpulseVector = J * _normal;

		glm::vec3 velocity = GetVelocity() + collisionImpulseVector * (1.0f / GetMass());
		SetVelocity(velocity);

		velocity = _other->GetVelocity() - collisionImpulseVector * (1.0f / _other->GetMass());
		_other->SetVelocity(velocity);

		glm::vec3 gravity_forceA = glm::vec3(0.0f, -9.81f * GetMass(), 0.0f);
		glm::vec3 normal_forceA = glm::dot(gravity_forceA, _normal) * _normal;
		AddForce(normal_forceA);

		glm::vec3 gravity_forceB = glm::vec3(0.0f, -9.81f * _other->GetMass(), 0.0f);
		glm::vec3 normal_forceB = glm::dot(gravity_forceB, _normal) * _normal;
		_other->AddForce(normal_forceB);

		glm::vec3 rA = _collisionPoint - GetPosition();
		glm::vec3 rB = _collisionPoint - _other->GetPosition();

		// Compute friction force
		glm::vec3 velA = GetVelocity() + glm::cross(GetAngularMomentum(), rA);
		glm::vec3 velB = _other->GetVelocity() + glm::cross(_other->GetAngularMomentum(), rB);
		glm::vec3 relative_velocity = velA - velB;
		float d_mu = 1.0f;
		glm::vec3 forward_relative_veclocity = relative_velocity - glm::dot(relative_velocity, _normal) * _normal;

		// Compute A rotation
		glm::vec3 friction_force = FrictionForce(relative_velocity, _normal, normal_forceA, d_mu);
		glm::vec3 torque_arm = rA;
		glm::vec3 torque = ComputeTorque(torque_arm, friction_force);
		// Add global damping
		torque -= GetAngularMomentum() * globalDamping;
		AddForce(friction_force);

		if (glm::length(forward_relative_veclocity) - glm::length(friction_force / mMass) * GetCore()->DeltaTime() > 0.0f)
		{
			AddForce(-friction_force);
			AddTorque(torque);
		}

		// Compute B rotation
		glm::vec3 friction_forceB = FrictionForce(relative_velocity, _normal, normal_forceB, d_mu);
		glm::vec3 torque_armB = rB;
		glm::vec3 torqueB = ComputeTorque(torque_armB, friction_forceB);
		// Add global damping
		torqueB -= _other->GetAngularMomentum() * globalDamping;
		_other->AddForce(friction_forceB);

		if (glm::length(forward_relative_veclocity) - glm::length(friction_forceB / _other->GetMass()) * GetCore()->DeltaTime() > 0.0f)
		{
			_other->AddForce(-friction_forceB);
			_other->AddTorque(torqueB);
		}
	}

	void Rigidbody::ApplyImpulseResponseStatic(glm::vec3 _normal, glm::vec3 _collisionPoint)
	{
		float _otherMass = 1000000.0f;

		glm::vec3 velocityA = GetVelocity();
		glm::vec3 velocityB = glm::vec3{ 0 };
		glm::vec3 relativeVelocity = velocityA - velocityB;

		float elasticity = 0.6f * 0.5f;
		float J_numerator = -(1.0f + elasticity) * glm::dot(relativeVelocity, _normal);
		float totalInverseMass = (1.0f / GetMass()) + (1.0f / _otherMass);
		float J = J_numerator / (totalInverseMass);

		glm::vec3 collisionImpulseVector = J * _normal;

		glm::vec3 velocity = GetVelocity() + collisionImpulseVector * (1.0f / GetMass());
		SetVelocity(velocity);

		glm::vec3 gravity_forceA = glm::vec3(0.0f, 9.81f * GetMass(), 0.0f);
		glm::vec3 normal_forceA = glm::dot(gravity_forceA, _normal) * _normal;
		AddForce(normal_forceA);

		glm::vec3 gravity_forceB = glm::vec3(0.0f, 9.81f * _otherMass, 0.0f);
		glm::vec3 normal_forceB = glm::dot(gravity_forceB, _normal) * _normal;

		glm::vec3 rA = _collisionPoint - GetPosition();

		// Compute friction force
		glm::vec3 velA = GetVelocity() + glm::cross(GetAngularMomentum(), rA);
		glm::vec3 relative_velocity = velA;
		float d_mu = 1.0f;
		glm::vec3 forward_relative_veclocity = relative_velocity - glm::dot(relative_velocity, _normal) * _normal;

		// Compute A rotation
		glm::vec3 friction_force = FrictionForce(relative_velocity, _normal, normal_forceA, d_mu);
		glm::vec3 torque_arm = rA;
		glm::vec3 torque = ComputeTorque(torque_arm, friction_force);
		// Add global damping
		torque -= GetAngularMomentum() * globalDamping;
		AddForce(friction_force);

		if (glm::length(forward_relative_veclocity) - glm::length(friction_force / mMass) * GetCore()->DeltaTime() > 0.0f)
		{
			AddForce(-friction_force);
			AddTorque(torque);
		}
	}


	glm::vec3 Rigidbody::FrictionForce(glm::vec3 _relativeVelocity, glm::vec3 _contactNormal, glm::vec3 _forceNormal, float mu)
	{
		glm::vec3 forward_relative_velocity = _relativeVelocity - glm::dot(_relativeVelocity, _contactNormal) * _contactNormal;
		float tangent_length = glm::length(forward_relative_velocity);

		if (tangent_length > 1e-6f) //0.000001
		{
			// Get normalised vector as the direction of travel
			glm::vec3 forward_direction = glm::normalize(forward_relative_velocity);

			// Friction direction is in the opposite direction of travel
			glm::vec3 friction_direction = -forward_direction;
			glm::vec3 friction_force = friction_direction * mu * glm::length(_forceNormal);
			return friction_force;
		}

		return glm::vec3(0);
	}

	glm::vec3 Rigidbody::ComputeTorque(glm::vec3 torque_arm, glm::vec3 contact_force)
	{
		glm::vec3 torque = glm::cross(contact_force, torque_arm);

		return torque;
	}

	void Rigidbody::Euler()
	{
		float oneOverMass = 1 / mMass;
		// Compute the current velocity based on the previous velocity
		mVelocity += (mForce * oneOverMass) * GetCore()->DeltaTime();
		// Compute the current position based on the previous position
		Move(mVelocity * GetCore()->DeltaTime());

		// Angular motion update
		mAngularMomentum += mTorque * GetCore()->DeltaTime();
		ComputeInverseInertiaTensor();
		// Update angular velocity
		mAngularVelocity = mInertiaTensorInverse * mAngularMomentum;
		// Construct skew matrix omega star
		glm::mat3 omega_star = glm::mat3(0.0f, -mAngularVelocity.z, mAngularVelocity.y,
			mAngularVelocity.z, 0.0f, -mAngularVelocity.x,
			-mAngularVelocity.y, mAngularVelocity.x, 0.0f);
		// Update rotation matrix
		mR += omega_star * mR * GetCore()->DeltaTime();
	}

	//void Rigidbody::Verlet()
	//{
	//	glm::vec3 acceleration = mForce / mMass;
	//	mPreviousPosition = transform.position - m_velocity * deltaTs + 0.5f * acceleration * deltaTs * deltaTs;
	//	transform.position = 2.0f * transform.position - m_previousPosition + acceleration * deltaTs * deltaTs;
	//	m_velocity = (transform.position - m_previousPosition) / (2.0f * deltaTs);
	//	m_velocity += acceleration * deltaTs;

	//	// Angular motion update
	//	m_angular_momentum += m_torque * deltaTs;
	//	computeInverseInertiaTensor();
	//	// Update angular velocity
	//	m_angular_velocity = m_inertia_tensor_inverse * m_angular_momentum;
	//	// Construct skew matrix omega star
	//	glm::mat3 omega_star = glm::mat3(0.0f, -m_angular_velocity.z, m_angular_velocity.y,
	//		m_angular_velocity.z, 0.0f, -m_angular_velocity.x,
	//		-m_angular_velocity.y, m_angular_velocity.x, 0.0f);
	//	// Update rotation matrix
	//	m_R += omega_star * m_R * deltaTs;
	//}

	void Rigidbody::CalculateEulerAngles()
	{
		float value = mR[0][0] * mR[0][0] + mR[1][0] * mR[1][0];
		float sy = sqrt(value);

		bool singular = sy < 1e-6;

		float x, y, z;

		if (!singular)
		{
			x = atan2(mR[2][1], mR[2][2]);
			y = atan2(-mR[2][0], sy);
			z = atan2(mR[1][0], mR[0][0]);
		}
		else
		{
			x = atan2(-mR[1][2], mR[1][1]);
			y = atan2(-mR[2][0], sy);
			z = 0;
		}

		float degrees = 180.0f / 3.1415f;

		SetRotation(glm::vec3(x * degrees, y * degrees, z * degrees));
	}

	void Rigidbody::UpdateInertiaTensor()
	{
		glm::mat3 bodyInertia = GetEntity()->GetComponent<Collider>()->UpdateInertiaTensor(mMass);

		mBodyInertiaTensorInverse = glm::inverse(bodyInertia);

		ComputeInverseInertiaTensor();
	}

	void Rigidbody::ComputeInverseInertiaTensor()
	{
		mInertiaTensorInverse = mR * mBodyInertiaTensorInverse * glm::transpose(mR);
	}

}