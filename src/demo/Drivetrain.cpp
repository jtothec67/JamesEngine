#include "Drivetrain.h"

namespace JamesEngine
{

	glm::vec2 Drivetrain::SplitTorque(float _torque, glm::vec2 _angularVelocityLR)
	{
        // Base open-diff split: 50/50
        const float baseEachNm = 0.5f * _torque;

        // Fast path: pure open diff if all LSD knobs are zero
        if (mPreloadNM == 0.0f && mKPower == 0.0f && mKCoast == 0.0f && mcVisc == 0.0f)
            return { baseEachNm, baseEachNm };

        // Relative wheel speed (rad/s): positive if left spins faster than right
        const float deltaOmega = _angularVelocityLR.x - _angularVelocityLR.y;

        // Viscous locking request opposes slip (Nm)
        const float viscousRequestNm = -mcVisc * deltaOmega;

        // Available locking capacity this instant (Nm)
        // Ramp with input torque: use kPower for drive (+), kCoast for engine braking (-)
        const float rampNm = (_torque >= 0.0f) ? (mKPower * _torque)
            : (mKCoast * -_torque);
        float lockCapacityNm = mPreloadNM + rampNm;
        if (lockCapacityNm < 0.0f) lockCapacityNm = 0.0f; // safety

        // Clamp the requested locking to available capacity
        const float lockNm = glm::clamp(viscousRequestNm, -lockCapacityNm, +lockCapacityNm);

        // Apply equal/opposite about the diff’s relative DOF
        const float leftNm = baseEachNm + lockNm;
        const float rightNm = baseEachNm - lockNm;

        // Sum remains _torque by construction
        return { leftNm, rightNm };
	}

	glm::vec2 Drivetrain::ApplyOpenDiffKinematics(glm::vec2 _angularVelocityLR, glm::vec2 _inertiaLR, float _carrierAngularVelocity)
	{
        float wL = _angularVelocityLR.x;
        float wR = _angularVelocityLR.y;

        // Protect against degenerate inertias
        const float IL = (_inertiaLR.x > 1e-6f) ? _inertiaLR.x : 1e-6f;
        const float IR = (_inertiaLR.y > 1e-6f) ? _inertiaLR.y : 1e-6f;

        // Constraint violation: g = (wL + wR) - 2*wC
        const float g = (wL + wR) - 2.0f * _carrierAngularVelocity;

        // Effective mass with carrier held fixed: k = 1/IL + 1/IR
        const float K = (1.0f / IL) + (1.0f / IR);
        if (K <= 0.0f) return _angularVelocityLR; // safety

        // Lagrange multiplier for a 1D velocity constraint
        const float lambda = -g / K;

        // Velocity corrections
        wL += lambda / IL;
        wR += lambda / IR;

        return { wL, wR };
	}

}