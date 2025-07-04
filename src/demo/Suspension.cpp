#include "Suspension.h"

#ifdef _DEBUG
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

namespace JamesEngine
{

#ifdef _DEBUG
	void Suspension::OnGUI()
	{
        if (!mDebugVisual)
            return;

        // Retrieve the current camera from the entity's core
        std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();

        // Set up the shader uniforms for projection and view matrices
        mShader->uniform("projection", camera->GetProjectionMatrix());
        mShader->uniform("view", camera->GetViewMatrix());

		if (!mWheel || !mAnchorPoint)
			return;

        // Get the anchor and wheel positions for the suspension
        glm::vec3 anchorPos = mAnchorPoint->GetComponent<Transform>()->GetPosition(); 
        glm::vec3 wheelPos = mWheel->GetComponent<Transform>()->GetPosition();

        // Compute the midpoint between the anchor and the wheel.
        glm::vec3 midPoint = (anchorPos + wheelPos) * 0.5f;

        // Compute the direction and length from the anchor to the wheel.
        glm::vec3 suspensionDir = wheelPos - anchorPos;
        float springLength = glm::length(suspensionDir);
        suspensionDir = glm::normalize(suspensionDir);

        // Since the spring model is oriented upward along the y-axis,
        // we need to compute the rotation that aligns (0,1,0) with suspensionDir.
        glm::vec3 up = glm::vec3(0, 1, 0);
        float dotVal = glm::dot(up, suspensionDir);
        glm::vec3 rotationAxis = glm::cross(up, suspensionDir);
        float angle = acos(glm::clamp(dotVal, -1.0f, 1.0f));

        // Handle the case where the vectors are nearly parallel or opposite.
        if (glm::length(rotationAxis) < 1e-6f) {
            if (dotVal < 0.0f) {
                // Vectors are opposite, so choose an arbitrary perpendicular axis.
                rotationAxis = glm::vec3(1, 0, 0);
                angle = glm::pi<float>();
            }
        }
        else {
            rotationAxis = glm::normalize(rotationAxis);
        }

        // Build the model matrix in several stages.
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        // 1. Translate to the midpoint of the suspension.
        modelMatrix = glm::translate(modelMatrix, midPoint);

        // 2. Rotate the spring model so that its local up (0,1,0) is aligned with the suspension direction.
        modelMatrix = modelMatrix * glm::rotate(glm::mat4(1.0f), angle, rotationAxis);

        // 3. Adjust the spring model so that it is centered:
        //    a. Translate it by (0, -0.5, 0) since the model is assumed to span [0,1] in y (its center is at y = 0.5).
        //    b. Scale in y so that the spring spans the full distance between the anchor and the wheel.
        glm::mat4 T_center = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.5f, 0));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, springLength, 1.0f));
        glm::mat4 localTransform = S * T_center;

        // Combine the transformations.
        modelMatrix = modelMatrix * localTransform;

        // Set the final model matrix to the shader.
        mShader->uniform("model", modelMatrix);

        // Additional uniforms for visual debugging (these can be adjusted for your needs).
        mShader->uniform("outlineWidth", 1.0f);
        mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

        // Disable depth testing so the debug visual is fully visible.
        glDisable(GL_DEPTH_TEST);

        // Draw the spring model (assumes mSpringModel holds the proper model).
        mShader->drawOutline(mModel.get());

        // Re-enable depth testing.
        glEnable(GL_DEPTH_TEST);
	}
#endif

    void Suspension::OnAlive()
    {
        if (!mCarBody || !mWheel || !mAnchorPoint)
        {
            std::cout << "Suspension component is missing a car body, wheel, or anchor point" << std::endl;
            return;
        }
        mCarRb = mCarBody->GetComponent<Rigidbody>();

        if (!mCarRb)
        {
            std::cout << "Suspension component is missing a rigidbody on the car" << std::endl;
            return;
        }
    }

    void Suspension::OnEarlyFixedTick()
    {
        // Get anchor position and suspension direction
        glm::vec3 anchorPos = mAnchorPoint->GetComponent<Transform>()->GetPosition();
        glm::vec3 suspensionDir = -glm::normalize(mAnchorPoint->GetComponent<Transform>()->GetUp());

        // Setup raycast
        Ray ray;
        ray.origin = anchorPos;
        ray.direction = suspensionDir;
        ray.length = mRestLength + mTireRadius;

        RaycastHit hit;
        mGroundContact = GetCore()->GetRaycastSystem()->Raycast(ray, hit);

        // Determine current suspension length
        if (mGroundContact)
        {
            mContactPoint = hit.point;
            mSurfaceNormal = hit.normal;
            mCurrentLength = hit.distance - mTireRadius;
        }
        else
        {
            mCurrentLength = mRestLength;
        }

        float targetLength = glm::clamp(mRideHeight + mTireRadius, 0.0f, mRestLength);
        mDisplacement = targetLength - mCurrentLength;
    }

    void Suspension::OnFixedTick()
    {
        glm::vec3 anchorPos = mAnchorPoint->GetComponent<Transform>()->GetPosition();
        glm::vec3 suspensionDir = -glm::normalize(mAnchorPoint->GetComponent<Transform>()->GetUp());

        // Update wheel position using previously calculated mCurrentLength
        glm::vec3 wheelPos = anchorPos + suspensionDir * mCurrentLength;
        mWheel->GetComponent<Transform>()->SetPosition(wheelPos);

        // Exit early if no ground contact
        if (!mGroundContact)
            return;

        float targetLength = glm::clamp(mRideHeight + mTireRadius, 0.0f, mRestLength);

        // Get velocity of anchor point projected along suspension direction
        glm::vec3 pointVelocity = mCarRb->GetVelocityAtPoint(anchorPos);
        float relativeVelocity = glm::dot(pointVelocity, suspensionDir);

        // Apply spring force
        float springForce = 0.0f;

        // Regular spring force within normal range
        if (mCurrentLength <= mRestLength && mCurrentLength >= 0.0f)
        {
            springForce = glm::max(0.0f, mStiffness * mDisplacement);
        }

        // Bump stop (bottoming out)
        if (mCurrentLength < mBumpStopRange)
        {
            float compressionDepth = mBumpStopRange - mCurrentLength;
            springForce += mBumpStopStiffness * compressionDepth;
        }

        // Bump stop (extension limit)
        if (mCurrentLength > mRestLength)
        {
            float extensionOverrun = mCurrentLength - mRestLength;
            if (extensionOverrun < mBumpStopRange)
            {
                float bumpCompression = mBumpStopRange - extensionOverrun;
                springForce += mBumpStopStiffness * bumpCompression;
            }
        }

        // Damping logic
        bool isRebound = (relativeVelocity > 0.0f);
        float absVel = std::abs(relativeVelocity);
        bool isHighSpeed = (absVel > mDampingThreshold);

        float dampingCoef = 0.0f;
        if (isRebound)
        {
            dampingCoef = isHighSpeed ? mReboundDampHighSpeed : mReboundDampLowSpeed;
        }
        else
        {
            dampingCoef = isHighSpeed ? mBumpDampHighSpeed : mBumpDampLowSpeed;
        }

        float dampingForce = -dampingCoef * relativeVelocity;

        float totalMag = springForce - dampingForce;

        glm::vec3 totalForce = -suspensionDir * totalMag;

        // Apply force to car body
        mCarRb->ApplyForce(totalForce, anchorPos);

        mForce = totalMag; // Stiffness + damping

        // If anti-roll bar is active, include its contribution
        if (mOppositeAxelSuspension)
        {
            float displacementDiff = mDisplacement - mOppositeAxelSuspension->GetDisplacement();
            float antiRollForce = mAntiRollBarStiffness * displacementDiff;

            glm::vec3 antiRollCorrection = -suspensionDir * antiRollForce;
            mCarRb->ApplyForce(antiRollCorrection, anchorPos);

            mForce += antiRollForce;
        }

		//std::cout << GetEntity()->GetTag() << " Suspension Force: " << mForce << std::endl;
    }

    void Suspension::OnLateFixedTick()
    {
        // Reset contact
        mGroundContact = false;
    }

    void Suspension::OnTick()
    {
        // Apply steering rotation
        glm::quat anchorRotationQuat = mAnchorPoint->GetComponent<Transform>()->GetWorldRotation();
        glm::quat steeringQuat = glm::angleAxis(glm::radians(mSteeringAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat finalRotation = anchorRotationQuat * steeringQuat;
        mWheel->GetComponent<Transform>()->SetQuaternion(finalRotation);
    }

}