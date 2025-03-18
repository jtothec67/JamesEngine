#include "Rigidbody.h"

#include "Entity.h"
#include "Core.h"
#include "Component.h"
#include "Transform.h"
#include "Collider.h"
#include "BoxCollider.h"

#include <vector>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/orthonormalize.hpp>
#include <glm/gtx/quaternion.hpp>

namespace JamesEngine
{

	void Rigidbody::OnInitialize()
	{
		UpdateInertiaTensor();

		mR = glm::toMat4(GetQuaternion());
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
					float totalInverseMass = (1.0f / mMass) + (1.0f / otherRigidbody->GetMass());

					Move(penetrationDepth * collisionNormal * (1.0f / mMass) / totalInverseMass);

					otherRigidbody->Move(-penetrationDepth * collisionNormal * (1.0f / otherRigidbody->GetMass()) / totalInverseMass);

					ApplyImpulseResponse(otherRigidbody, collisionNormal, collisionPoint);
				}
				else // We are rigidbody, other isn't
				{
					Move(penetrationDepth * collisionNormal);

					ApplyImpulseResponseStatic(collisionNormal, collisionPoint);
				}
			}
		}

		// Step 5: Integration
		//Euler();
		Verlet();

		// Setp 6: Convert to euler angles
		CalculateAngles();

		// Step 7: Clear forces
		ClearForces();
	}

	void Rigidbody::ApplyImpulseResponse(std::shared_ptr<Rigidbody> _other, glm::vec3 _normal, glm::vec3 _collisionPoint)
	{
		//glm::vec3 velocityA = GetVelocity();
		//glm::vec3 velocityB = _other->GetVelocity();
		//glm::vec3 relativeVelocity = velocityA - velocityB;

		//float elasticity = 0.6f * 0.5f;
		//float J_numerator = -(1.0f + elasticity) * glm::dot(relativeVelocity, _normal);
		//float totalInverseMass = (1.0f / GetMass()) + (1.0f / _other->GetMass());
		//float J = J_numerator / (totalInverseMass);

		//glm::vec3 collisionImpulseVector = J * _normal;

		//glm::vec3 velocity = GetVelocity() + collisionImpulseVector * (1.0f / GetMass());
		//SetVelocity(velocity);

		//velocity = _other->GetVelocity() - collisionImpulseVector * (1.0f / _other->GetMass());
		//_other->SetVelocity(velocity);

		//// Compute lever arms (from center of mass to collision point)
		//glm::vec3 rA = _collisionPoint - GetPosition();
		//glm::vec3 rB = _collisionPoint - _other->GetPosition();
		//// Angular impulse is r × impulse.
		//glm::vec3 angularImpulseA = glm::cross(rA, collisionImpulseVector);
		//glm::vec3 angularImpulseB = glm::cross(rB, -collisionImpulseVector);
		//// Update angular momentum directly.
		//SetAngularMomentum(GetAngularMomentum() + angularImpulseA);
		//_other->SetAngularMomentum(_other->GetAngularMomentum() + angularImpulseB);

		//glm::vec3 gravity_forceA = glm::vec3(0.0f, -9.81f * GetMass(), 0.0f);
		//glm::vec3 normal_forceA = glm::dot(gravity_forceA, _normal) * _normal;
		//AddForce(normal_forceA);

		//glm::vec3 gravity_forceB = glm::vec3(0.0f, -9.81f * _other->GetMass(), 0.0f);
		//glm::vec3 normal_forceB = glm::dot(gravity_forceB, _normal) * _normal;
		//_other->AddForce(normal_forceB);

		//// Compute friction force
		//glm::vec3 velA = GetVelocity() + glm::cross(GetAngularMomentum(), rA);
		//glm::vec3 velB = _other->GetVelocity() + glm::cross(_other->GetAngularMomentum(), rB);
		//glm::vec3 relative_velocity = velA - velB;
		//float d_mu = 1.0f;
		//glm::vec3 forward_relative_veclocity = relative_velocity - glm::dot(relative_velocity, _normal) * _normal;

		//// Compute A rotation
		//glm::vec3 friction_force = FrictionForce(relative_velocity, _normal, normal_forceA, d_mu);
		//glm::vec3 torque_arm = rA;
		//glm::vec3 torque = ComputeTorque(torque_arm, friction_force);
		//// Add global damping
		//torque -= GetAngularMomentum() * globalDamping;
		//AddForce(friction_force);

		//if (glm::length(forward_relative_veclocity) - glm::length(friction_force / mMass) * GetCore()->DeltaTime() > 0.0f)
		//{
		//	AddForce(-friction_force);
		//	AddTorque(torque);
		//}

		//// Compute B rotation
		//glm::vec3 friction_forceB = FrictionForce(relative_velocity, _normal, normal_forceB, d_mu);
		//glm::vec3 torque_armB = rB;
		//glm::vec3 torqueB = ComputeTorque(torque_armB, friction_forceB);
		//// Add global damping
		//torqueB -= _other->GetAngularMomentum() * globalDamping;
		//_other->AddForce(friction_forceB);

		//if (glm::length(forward_relative_veclocity) - glm::length(friction_forceB / _other->GetMass()) * GetCore()->DeltaTime() > 0.0f)
		//{
		//	_other->AddForce(-friction_forceB);
		//	_other->AddTorque(torqueB);
		//}

		// --- Precompute values for both bodies ---
		float invMassA = (GetMass() > 0.0f) ? (1.0f / GetMass()) : 0.0f;
		float invMassB = (_other->GetMass() > 0.0f) ? (1.0f / _other->GetMass()) : 0.0f;

		// Compute lever arms from centers of mass to collision point.
		glm::vec3 ra = _collisionPoint - GetPosition();
		glm::vec3 rb = _collisionPoint - _other->GetPosition();

		// Calculate velocities at the contact point (including rotation).
		glm::vec3 vA = GetVelocity() + glm::cross(GetAngularVelocity(), ra);
		glm::vec3 vB = _other->GetVelocity() + glm::cross(_other->GetAngularVelocity(), rb);

		// Relative velocity at contact point.
		glm::vec3 relativeVel = vA - vB;
		float relVelAlongNormal = glm::dot(relativeVel, _normal);

		// Do not resolve if moving apart.
		//if (relVelAlongNormal > 0.0f)
			//return;

		// --- Effective mass (including rotation) for normal impulse ---
		// Compute the effect of angular inertia.
		glm::vec3 raCrossN = glm::cross(ra, _normal);
		glm::vec3 rbCrossN = glm::cross(rb, _normal);
		float angularEffectA = glm::dot(_normal, glm::cross(mInertiaTensorInverse * raCrossN, ra));
		float angularEffectB = glm::dot(_normal, glm::cross(_other->mInertiaTensorInverse * rbCrossN, rb));

		// Bounce factor (elasticity).
		float restitution = 1.f; // Adjust as needed.

		// Compute impulse scalar (j) for the collision along the normal.
		float j = -(1.0f + restitution) * relVelAlongNormal;
		j /= (invMassA + invMassB + angularEffectA + angularEffectB);

		// Normal impulse.
		glm::vec3 impulse = j * _normal;

		// Apply linear impulse.
		SetVelocity(GetVelocity() + impulse * invMassA);
		_other->SetVelocity(_other->GetVelocity() - impulse * invMassB);

		// Apply angular impulse.
		SetAngularMomentum(GetAngularMomentum() + glm::cross(ra, impulse));
		_other->SetAngularMomentum(_other->GetAngularMomentum() - glm::cross(rb, impulse));

		// --- Friction Impulse ---
		// Recompute contact velocities with updated velocities.
		vA = GetVelocity() + glm::cross(GetAngularVelocity(), ra);
		vB = _other->GetVelocity() + glm::cross(_other->GetAngularVelocity(), rb);
		relativeVel = vA - vB;

		// Compute the contact tangent direction.
		glm::vec3 tangent = relativeVel - glm::dot(relativeVel, _normal) * _normal;
		float tangentialSpeed = glm::length(tangent);
		if (tangentialSpeed > 1e-6f)
		{
			tangent = tangent / tangentialSpeed;
		}
		else
		{
			tangent = glm::vec3(0.0f);
		}

		// Effective mass for friction (similar to the normal calculation).
		glm::vec3 raCrossT = glm::cross(ra, tangent);
		glm::vec3 rbCrossT = glm::cross(rb, tangent);
		float angularEffectA_T = glm::dot(tangent, glm::cross(mInertiaTensorInverse * raCrossT, ra));
		float angularEffectB_T = glm::dot(tangent, glm::cross(_other->mInertiaTensorInverse * rbCrossT, rb));

		float jt = -glm::dot(relativeVel, tangent);
		jt /= (invMassA + invMassB + angularEffectA_T + angularEffectB_T);

		// Friction coefficient.
		float frictionCoefficient = 1.0f; // Adjust as needed.

		// Clamp friction to Coulomb’s friction law.
		glm::vec3 frictionImpulse;
		if (glm::abs(jt) <= j * frictionCoefficient)
			frictionImpulse = jt * tangent;
		else
			frictionImpulse = -j * frictionCoefficient * tangent;

		// Apply friction impulses.
		SetVelocity(GetVelocity() + frictionImpulse * invMassA);
		_other->SetVelocity(_other->GetVelocity() - frictionImpulse * invMassB);
		SetAngularMomentum(GetAngularMomentum() + glm::cross(ra, frictionImpulse));
		_other->SetAngularMomentum(_other->GetAngularMomentum() - glm::cross(rb, frictionImpulse));
	}

	void Rigidbody::ApplyImpulseResponseStatic(glm::vec3 _normal, glm::vec3 _collisionPoint)
	{
		//float _otherMass = 1000000.0f;

		//glm::vec3 velocityA = GetVelocity();
		//glm::vec3 velocityB = glm::vec3{ 0 };
		//glm::vec3 relativeVelocity = velocityA - velocityB;

		//float elasticity = 0.8f * 0.8f;
		//float J_numerator = -(1.0f + elasticity) * glm::dot(relativeVelocity, _normal);
		//float totalInverseMass = (1.0f / GetMass()) + (1.0f / _otherMass);
		//float J = J_numerator / (totalInverseMass);

		//glm::vec3 collisionImpulseVector = J * _normal;

		//glm::vec3 velocity = GetVelocity() + collisionImpulseVector / GetMass();
		//SetVelocity(velocity);

		//// Compute lever arm from center of mass to collision point.
		//glm::vec3 rA = _collisionPoint - GetPosition();
		//// Angular impulse = r × impulse.
		//glm::vec3 angularImpulse = glm::cross(rA, collisionImpulseVector);
		//// Update the dynamic object's angular momentum.
		//SetAngularMomentum(GetAngularMomentum() + angularImpulse);

		//glm::vec3 gravity_forceA = glm::vec3(0.0f, -9.81f * GetMass(), 0.0f);
		//glm::vec3 normal_forceA = glm::dot(gravity_forceA, _normal) * _normal;
		//AddForce(normal_forceA);

		//glm::vec3 gravity_forceB = glm::vec3(0.0f, -9.81f * _otherMass, 0.0f);
		//glm::vec3 normal_forceB = glm::dot(gravity_forceB, _normal) * _normal;

		//// Compute friction force
		//glm::vec3 velA = GetVelocity() + glm::cross(GetAngularMomentum(), rA);
		//glm::vec3 relative_velocity = velA;
		//float d_mu = 1.0f;
		//glm::vec3 forward_relative_veclocity = relative_velocity - glm::dot(relative_velocity, _normal) * _normal;

		//// Compute A rotation
		//glm::vec3 friction_force = FrictionForce(relative_velocity, _normal, normal_forceA, d_mu);
		//glm::vec3 torque_arm = rA;
		//glm::vec3 torque = ComputeTorque(torque_arm, friction_force);
		//// Add global damping
		//torque -= GetAngularMomentum() * globalDamping;
		//AddForce(friction_force);

		//if (glm::length(forward_relative_veclocity) - glm::length(friction_force / mMass) * GetCore()->DeltaTime() > 0.0f)
		//{
		//	AddForce(-friction_force);
		//	AddTorque(torque);
		//}

		// ----------------------------- Second Implementation ---------------------------
		//// --- Precompute dynamic body parameters ---
		//float invMass = (GetMass() > 0.0f) ? (1.0f / GetMass()) : 0.0f;

		//// Compute lever arm from center of mass to collision point.
		//glm::vec3 r = _collisionPoint - GetPosition();

		//// Compute the contact velocity (including rotational component).
		//glm::vec3 v = GetVelocity() + glm::cross(GetAngularVelocity(), r);

		//// Compute relative velocity along the collision normal.
		//float vRel = glm::dot(v, _normal);

		//// If the dynamic object is moving away from the contact, no impulse is applied.
		//if (vRel > 0.0f)
		//	return;

		//// --- Compute effective mass (including rotational inertia) for the normal impulse ---
		//glm::vec3 rCrossN = glm::cross(r, _normal);
		//float angularEffect = glm::dot(_normal, glm::cross(mInertiaTensorInverse * rCrossN, r));

		//// Restitution (elasticity) factor.
		//float restitution = 1.2f; // Adjust as needed.

		//// Compute impulse scalar along the normal.
		//float j = -(1.0f + restitution) * vRel;
		//j /= (invMass + angularEffect);

		//// Normal impulse vector.
		//glm::vec3 impulse = j * _normal;

		//// Update dynamic object's linear and angular momentum.
		//SetVelocity(GetVelocity() + impulse * invMass);
		//SetAngularMomentum(GetAngularMomentum() + glm::cross(r, impulse));

		//// --- Friction Impulse ---
		//// Recompute contact velocity after normal impulse.
		//v = GetVelocity() + glm::cross(GetAngularVelocity(), r);

		//// Extract tangent component of the contact velocity.
		//glm::vec3 tangent = v - glm::dot(v, _normal) * _normal;
		//float tangentSpeed = glm::length(tangent);
		//if (tangentSpeed > 1e-6f)
		//	tangent /= tangentSpeed;
		//else
		//	tangent = glm::vec3(0.0f);

		//// Effective mass for friction impulse.
		//glm::vec3 rCrossT = glm::cross(r, tangent);
		//float angularEffectT = glm::dot(tangent, glm::cross(mInertiaTensorInverse * rCrossT, r));

		//// Compute friction scalar impulse.
		//float jt = -glm::dot(v, tangent);
		//jt /= (invMass + angularEffectT);

		//// Friction coefficient (Coulomb friction law).
		//float frictionCoefficient = 1.0f; // Adjust as needed.

		//// Clamp friction impulse so that |j_friction| <= frictionCoefficient * |j|
		//glm::vec3 frictionImpulse = jt * tangent;
		//if (glm::length(frictionImpulse) > glm::abs(j) * frictionCoefficient)
		//	frictionImpulse = -glm::abs(j) * frictionCoefficient * tangent;

		//// Apply friction impulse to the dynamic object.
		//SetVelocity(GetVelocity() + frictionImpulse * invMass);
		//SetAngularMomentum(GetAngularMomentum() + glm::cross(r, frictionImpulse));

		// ----------------------------- Third Implementation ---------------------------
		// Compute the vector from the center of mass to the collision point.
		glm::vec3 r = _collisionPoint - GetPosition();
    
		// Calculate the velocity at the collision point (linear plus rotational).
		glm::vec3 v = GetVelocity() + glm::cross(GetAngularVelocity(), r);
    
		// Determine the component of the velocity along the collision normal.
		float velAlongNormal = glm::dot(v, _normal);
    
		// If the dynamic object is moving away from the static object, no impulse is needed.
		if (velAlongNormal > 0)
			return;
    
		// Use the object's restitution value.
		float e = 0.5f;
    
		// Since the static object doesn't move, its inverse mass is 0.
		float invMassSum = 1.0f / GetMass();
    
		// Compute the rotational inertia term.
		glm::vec3 r_cross_n = glm::cross(r, _normal);
		glm::vec3 angularComponent = mInertiaTensorInverse * r_cross_n;
		float angularTerm = glm::dot(_normal, glm::cross(angularComponent, r));
    
		// Calculate the impulse scalar.
		float j = -(1.0f + e) * velAlongNormal / (invMassSum + angularTerm);
    
		// Compute the impulse vector along the normal.
		glm::vec3 impulse = j * _normal;
    
		// Apply the impulse: update linear and angular velocities.
		SetVelocity(GetVelocity() + impulse * (1.0f / GetMass()));

		SetAngularMomentum(GetAngularMomentum() + glm::cross(r, impulse));
		SetAngularVelocity(GetAngularVelocity() + mInertiaTensorInverse * GetAngularMomentum());
    
		// --- Coulomb Friction ---
		// Recalculate the velocity at the collision point after applying the normal impulse.
		v = GetVelocity() + glm::cross(GetAngularVelocity(), r);
    
		// The static object has zero velocity, so the relative velocity is simply v.
		glm::vec3 relativeVelocity = v;
    
		// Determine the tangent direction by removing the normal component.
		glm::vec3 tangent = relativeVelocity - glm::dot(relativeVelocity, _normal) * _normal;
		if (glm::length(tangent) > 0.0001f)
			tangent = glm::normalize(tangent);
		else
			return; // No significant tangential motion; skip friction.
    
		// Calculate the friction impulse scalar.
		float jt = -glm::dot(relativeVelocity, tangent);
		jt /= (1.0f / GetMass()) + glm::dot(tangent, glm::cross(mInertiaTensorInverse * glm::cross(r, tangent), r));
    
		// Use the object's friction coefficient (static object assumed to have no friction properties).
		float mu = 0.9;
    
		// Clamp the friction impulse to the Coulomb friction cone.
		if (fabs(jt) > mu * fabs(j))
			jt = mu * fabs(j) * (jt < 0 ? -1.0f : 1.0f);
    
		// Compute the friction impulse vector.
		glm::vec3 frictionImpulse = jt * tangent;
    
		// Apply the friction impulse.
		SetVelocity(GetVelocity() + frictionImpulse * (1.0f / GetMass()));

		SetAngularMomentum(GetAngularMomentum() + glm::cross(r, frictionImpulse));
		SetAngularVelocity(GetAngularVelocity() + mInertiaTensorInverse * GetAngularMomentum());

		std::cout << "r: " << r.x << ", " << r.y << ", " << r.z << std::endl;
		std::cout << "impule: " << impulse.x << ", " << impulse.y << ", " << impulse.z << std::endl;
		std::cout << "cross product: " << glm::cross(r, impulse).x << ", " << glm::cross(r, impulse).y << ", " << glm::cross(r, impulse).z << std::endl;
		std::cout << "GetAngularVelocity(): " << GetAngularVelocity().x << ", " << GetAngularVelocity().y << ", " << GetAngularVelocity().z << std::endl;
	}

	glm::vec3 Rigidbody::FrictionForce(glm::vec3 _relativeVelocity, glm::vec3 _contactNormal, glm::vec3 _forceNormal, float mu)
	{
		glm::vec3 tangential = _relativeVelocity - glm::dot(_relativeVelocity, _contactNormal) * _contactNormal;
		float speed = glm::length(tangential);

		if (speed > 1e-6f)
		{
			// Friction acts opposite to the sliding direction.
			glm::vec3 frictionDirection = -glm::normalize(tangential);
			// Coulomb friction: magnitude = mu * |normal force|
			return frictionDirection * mu * glm::length(_forceNormal);
		}

		return glm::vec3(0.0f);
	}

	glm::vec3 Rigidbody::ComputeTorque(glm::vec3 torque_arm, glm::vec3 contact_force)
	{
		glm::vec3 torque = glm::cross(contact_force, torque_arm);

		return torque;
	}

	void Rigidbody::Euler()
	{
		//float oneOverMass = 1 / mMass;
		//// Compute the current velocity based on the previous velocity
		//mVelocity += (mForce * oneOverMass) * GetCore()->DeltaTime();
		//// Compute the current position based on the previous position
		//Move(mVelocity * GetCore()->DeltaTime());

		//// Angular motion update
		//mAngularMomentum += mTorque * GetCore()->DeltaTime();
		//ComputeInverseInertiaTensor();
		//// Update angular velocity
		//mAngularVelocity = mInertiaTensorInverse * mAngularMomentum;
		//// Construct skew matrix omega star
		//glm::mat3 omega_star = glm::mat3(0.0f, -mAngularVelocity.z, mAngularVelocity.y,
		//	mAngularVelocity.z, 0.0f, -mAngularVelocity.x,
		//	-mAngularVelocity.y, mAngularVelocity.x, 0.0f);
		//// Update rotation matrix
		//mR += omega_star * mR * GetCore()->DeltaTime();
		// Get the time step.
		float dt = GetCore()->DeltaTime();

		// --- Linear Dynamics ---
		// Compute inverse mass (if mass is zero, the body is static).
		float invMass = (mMass > 0.0f) ? (1.0f / mMass) : 0.0f;

		// Update linear velocity from forces.
		mVelocity += (mForce * invMass) * dt;
		// Update position.
		Move(mVelocity * dt);

		// --- Angular Dynamics ---
		// Update angular momentum from applied torques.
		mAngularMomentum += mTorque * dt;

		// Recompute inverse inertia tensor in world space.
		// (ComputeInverseInertiaTensor() should update mInertiaTensorInverse using mR and the body inertia tensor.)
		ComputeInverseInertiaTensor();

		// Update angular velocity.
		mAngularVelocity = mInertiaTensorInverse * mAngularMomentum;

		// Build the skew-symmetric matrix for angular velocity.
		glm::mat3 omegaStar(0.0f);
		omegaStar[0][1] = -mAngularVelocity.z;
		omegaStar[0][2] = mAngularVelocity.y;
		omegaStar[1][0] = mAngularVelocity.z;
		omegaStar[1][2] = -mAngularVelocity.x;
		omegaStar[2][0] = -mAngularVelocity.y;
		omegaStar[2][1] = mAngularVelocity.x;

		// Update the rotation matrix (Euler integration on SO(3)).
		mR += omegaStar * mR * dt;
		// Re-orthonormalize the rotation matrix to prevent drift.
		mR = glm::orthonormalize(mR);
	}

	void Rigidbody::Verlet()
	{
		float dt = GetCore()->DeltaTime();

		glm::vec3 acceleration = mForce / mMass;
		mPreviousPosition = GetPosition() - mVelocity * dt + 0.5f * acceleration * dt * dt;
		SetPosition(2.0f * GetPosition() - mPreviousPosition + acceleration * dt * dt);
		mVelocity = (GetPosition() - mPreviousPosition) / (2.0f * dt);
		mVelocity += acceleration * dt;

		// Angular motion update
		mAngularMomentum += mTorque * dt;
		ComputeInverseInertiaTensor();
		// Update angular velocity
		mAngularVelocity = mInertiaTensorInverse * mAngularMomentum;
		// Construct skew matrix omega star
		glm::mat3 omegaStar(0.0f);
		omegaStar[0][1] = -mAngularVelocity.z;
		omegaStar[0][2] = mAngularVelocity.y;
		omegaStar[1][0] = mAngularVelocity.z;
		omegaStar[1][2] = -mAngularVelocity.x;
		omegaStar[2][0] = -mAngularVelocity.y;
		omegaStar[2][1] = mAngularVelocity.x;
		// Update rotation matrix
		mR += omegaStar * mR * dt;

		mR = glm::orthonormalize(mR);
	}

	void Rigidbody::CalculateAngles()
	{
		glm::quat q = glm::quat_cast(mR);
		q = glm::conjugate(q);
		SetQuaternion(q);
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