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

        std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();
        mShader->uniform("projection", camera->GetProjectionMatrix());
        mShader->uniform("view", camera->GetViewMatrix());

        if (!mWheel || !mAnchorPoint)
            return;

        // Anchor basis and ray direction (cast along -Up)
        glm::vec3 anchorPos = mAnchorPoint->GetComponent<Transform>()->GetPosition();
        glm::vec3 anchorUp = mAnchorPoint->GetComponent<Transform>()->GetUp();
        glm::vec3 anchorRight = mAnchorPoint->GetComponent<Transform>()->GetRight();
        glm::vec3 anchorForward = mAnchorPoint->GetComponent<Transform>()->GetForward();
        glm::vec3 rayDir = -anchorUp;

        // Length for viz: draw to hit if we have one, else full ray
        float fullRayLength = mSuspensionParams.restLength + mTireRadius;
        float drawRayLength = mGroundContact ? (mCurrentLength + mTireRadius) : fullRayLength;

        // Rotate offset basis by steering angle (must match physics)
        glm::quat steerQuat = glm::angleAxis(glm::radians(mSteeringAngle), anchorUp);
        glm::vec3 steerFwd = steerQuat * anchorForward;
        glm::vec3 steerRight = steerQuat * anchorRight;

        // Offsets: center + 4 corners (using steered basis)
        float latOffset = 0.4f * mTireWidth;
        float longOffset = 0.3f * mTireRadius;
        glm::vec3 offsets[5] = {
            glm::vec3(0.0f),
            steerRight * latOffset + steerFwd * longOffset,
            steerRight * latOffset - steerFwd * longOffset,
            -steerRight * latOffset + steerFwd * longOffset,
            -steerRight * latOffset - steerFwd * longOffset
        };

        // Common capsule settings
        float thickness = 0.03f; // skinny capsule (X/Z), tweak as desired
        glm::vec3 capsuleUp = glm::vec3(0, 1, 0);

        // Precompute rotation to align capsule +Y to rayDir
        float dotVal = glm::clamp(glm::dot(capsuleUp, rayDir), -1.0f, 1.0f);
        glm::vec3 rotAxis = glm::cross(capsuleUp, rayDir);
        float angle = std::acos(dotVal);
        if (glm::length(rotAxis) < 1e-12f) {
            if (dotVal < 0.0f) { rotAxis = glm::vec3(1, 0, 0); angle = glm::pi<float>(); }
            else { rotAxis = glm::vec3(1, 0, 0); angle = 0.0f; }
        }
        else {
            rotAxis = glm::normalize(rotAxis);
        }

        // Visual style
        mShader->uniform("outlineWidth", 1.0f);
        mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

        glDisable(GL_DEPTH_TEST);

        // Draw all 5 capsules
        for (int i = 0; i < 5; ++i)
        {
            glm::vec3 origin = anchorPos + offsets[i];
            glm::vec3 midPoint = origin + 0.5f * drawRayLength * rayDir;

            glm::mat4 model(1.0f);
            model = glm::translate(model, midPoint);
            model = model * glm::rotate(glm::mat4(1.0f), angle, rotAxis);
            // Capsule spans [-1..+1] in local Y; scale Y by drawRayLength/2
            model = model * glm::scale(glm::mat4(1.0f),
                glm::vec3(thickness, drawRayLength * 0.5f, thickness));

            mShader->uniform("model", model);
            mShader->drawOutline(mModel.get()); // capsule mesh
        }

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

		// Lots more things that would break if they weren't set, but a small check to make sure you haven't forgotten to set any params
        if (mSuspensionParams.stiffness == 0)
        {
            std::cout << "Suspension parameters are not set correctly" << std::endl;
            return;
		}
    }

    void Suspension::OnEarlyFixedTick()
    {
        // Get anchor position and suspension direction
        glm::vec3 anchorPos = mAnchorPoint->GetComponent<Transform>()->GetPosition();
        glm::vec3 anchorUp = mAnchorPoint->GetComponent<Transform>()->GetUp();
		glm::vec3 anchorForward = mAnchorPoint->GetComponent<Transform>()->GetForward();
		glm::vec3 anchorRight = mAnchorPoint->GetComponent<Transform>()->GetRight();
        glm::vec3 suspensionDir = -anchorUp;

        float rayLength = mSuspensionParams.restLength + mTireRadius;

        // Rotate the offset basis by steering angle around the anchor's Up
        glm::quat steerQuat = glm::angleAxis(glm::radians(mSteeringAngle), anchorUp);
        glm::vec3 steerFwd = steerQuat * anchorForward;
        glm::vec3 steerRight = steerQuat * anchorRight;

        // 5 offsets: center + 4 corners
        float latOffset = 0.4f * mTireWidth;
        float longOffset = 0.3f * mTireRadius;
        glm::vec3 offsets[5] = {
        glm::vec3(0.0f),
        steerRight * latOffset + steerFwd * longOffset,
        steerRight * latOffset - steerFwd * longOffset,
        -steerRight * latOffset + steerFwd * longOffset,
        -steerRight * latOffset - steerFwd * longOffset
        };

        int hitCount = 0;
        glm::vec3 sumPoints(0.0f);
        glm::vec3 sumNormals(0.0f);
        float sumLengths = 0.0f;

        for (int i = 0; i < 5; ++i)
        {
            Ray ray;
            ray.origin = anchorPos + offsets[i];
            ray.direction = suspensionDir;
            ray.length = rayLength;

            RaycastHit hit;
            if (GetCore()->GetRaycastSystem()->Raycast(ray, hit))
            {
                hitCount++;
                sumPoints += hit.point;
                sumNormals += hit.normal;
                sumLengths += hit.distance - mTireRadius;
            }
        }

        if (hitCount > 0)
        {
            mGroundContact = true;
            mContactPoint = sumPoints / (float)hitCount;
            mSurfaceNormal = glm::normalize(sumNormals / (float)hitCount);
            mCurrentLength = sumLengths / (float)hitCount;
        }
        else
        {
            mGroundContact = false;
            mCurrentLength = mSuspensionParams.restLength;
        }

        float targetLength = glm::clamp(mSuspensionParams.rideHeight + mTireRadius, 0.0f, mSuspensionParams.restLength);
        mDisplacement = targetLength - mCurrentLength;
    }

    void Suspension::OnFixedTick()
    {
        glm::vec3 anchorPos = mAnchorPoint->GetComponent<Transform>()->GetPosition();
        glm::vec3 suspensionDir = -glm::normalize(mAnchorPoint->GetComponent<Transform>()->GetUp());

		float wheelDistance = mCurrentLength;

        if (mCurrentLength < mTireRadius)
			wheelDistance = mTireRadius;

        // Update wheel position using previously calculated mCurrentLength
        glm::vec3 wheelPos = anchorPos + suspensionDir * wheelDistance;
        mWheel->GetComponent<Transform>()->SetPosition(wheelPos);

        // Exit early if no ground contact
        if (!mGroundContact)
            return;

        float targetLength = glm::clamp(mSuspensionParams.rideHeight + mTireRadius, 0.0f, mSuspensionParams.restLength);

        // Get velocity of anchor point projected along suspension direction
        glm::vec3 pointVelocity = mCarRb->GetVelocityAtPoint(anchorPos);
        float relativeVelocity = glm::dot(pointVelocity, suspensionDir);

        // Apply spring force
        float springForce = 0.0f;

        // Regular spring force within normal range
        if (mCurrentLength <= mSuspensionParams.restLength && mCurrentLength >= 0.0f)
        {
            springForce = glm::max(0.0f, mSuspensionParams.stiffness * mDisplacement);
        }

        // Bump stop (bottoming out)
        if (mCurrentLength < mSuspensionParams.bumpStopRange)
        {
            float compressionDepth = mSuspensionParams.bumpStopRange - mCurrentLength;
            springForce += mSuspensionParams.bumpStopStiffness * compressionDepth;
        }

        // Bump stop (extension limit)
        if (mCurrentLength > mSuspensionParams.restLength)
        {
            float extensionOverrun = mCurrentLength - mSuspensionParams.restLength;
            if (extensionOverrun < mSuspensionParams.bumpStopRange)
            {
                float bumpCompression = mSuspensionParams.bumpStopRange - extensionOverrun;
                springForce += mSuspensionParams.bumpStopStiffness * bumpCompression;
            }
        }

        // Damping logic
        bool isRebound = (relativeVelocity > 0.0f);
        float absVel = std::abs(relativeVelocity);
        bool isHighSpeed = (absVel > mSuspensionParams.dampingThreshold);

        float dampingCoef = 0.0f;
        if (isRebound)
        {
            dampingCoef = isHighSpeed ? mSuspensionParams.reboundDampHighSpeed : mSuspensionParams.reboundDampLowSpeed;
        }
        else
        {
            dampingCoef = isHighSpeed ? mSuspensionParams.bumpDampHighSpeed : mSuspensionParams.bumpDampLowSpeed;
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
            float antiRollForce = mSuspensionParams.antiRollBarStiffness * displacementDiff;

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