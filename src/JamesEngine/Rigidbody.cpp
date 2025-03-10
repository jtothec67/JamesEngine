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
		for (auto otherCollider : colliders)
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
				// Call OnCOllision for all components on both entities
				for (size_t ci = 0; ci < GetEntity()->mComponents.size(); ci++)
				{
					GetEntity()->mComponents.at(ci)->OnCollision(otherCollider->GetEntity());
				}

				for (size_t ci = 0; ci < otherCollider->GetEntity()->mComponents.size(); ci++)
				{
					otherCollider->GetEntity()->mComponents.at(ci)->OnCollision(GetEntity());
				}

				bool otherIsRigidbody = false;
				std::shared_ptr<Rigidbody> otherRigidbody = std::dynamic_pointer_cast<Rigidbody>(otherCollider);

				if (otherRigidbody)
					otherIsRigidbody = true;

				// Step 3: Compute responses

			}
		}

		// Step 4: Integration
		Euler();

		// Setp 5: Convert to euler angles
		CalculateEulerAngles();

		// Step 5: Clear forces
		ClearForces();
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
		glm::vec3 angles;

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