#include "Suspension.h"

#include "Core.h"
#include "Entity.h"
#include "Transform.h"
#include "Rigidbody.h"

#ifdef _DEBUG
#include "Camera.h"

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

    void Suspension::OnFixedTick()
    {
        if (!mWheel || !mCarBody || !mAnchorPoint)
        {
            std::cout << "Suspension component is missing a wheel, car body, or anchor point" << std::endl;
            return;
        }

        if (!mWheelRb || !mCarRb)
        {
            mWheelRb = mWheel->GetComponent<Rigidbody>();
            mCarRb = mCarBody->GetComponent<Rigidbody>();

            if (!mWheelRb || !mCarRb)
            {
                std::cout << "Suspension component is missing a rigidbody on the wheel or car body" << std::endl;
                return;
            }
        }

        //     // Get positions and the suspension axis (local y of the anchor)
        //     std::shared_ptr<Transform> anchorTransform = mAnchorPoint->GetComponent<Transform>();
        //     std::shared_ptr<Transform> wheelTransform = mWheel->GetComponent<Transform>();
        //     glm::vec3 anchorPos = anchorTransform->GetPosition();
        //     glm::vec3 wheelPos = wheelTransform->GetPosition();
        //     glm::vec3 suspensionAxis = glm::normalize(anchorTransform->GetUp());

        //     // Compute the offset and the component along the suspension axis.
        //     glm::vec3 offset = wheelPos - anchorPos;
        //     float currentLength = glm::dot(offset, suspensionAxis);
        //     // Clamp the suspension length between min and max values
        //     float clampedLength = glm::clamp(currentLength, mMinLength, mMaxLength);
        //     float compression = glm::max(0.0f, mRestLength - clampedLength);
        //     //float compression = mRestLength - currentLength;

        //     // Compute relative velocity along the suspension axis
        //     glm::vec3 wheelVel = mWheelRb->GetVelocity();
        //     glm::vec3 carVel = mCarRb->GetVelocityAtPoint(anchorPos);
        //     float relativeVel = glm::dot(wheelVel - carVel, suspensionAxis);

        //     // Calculate the spring and damper contributions
        //     float springForceMag = mStiffness * compression;
        //     float damperForceMag = -mDamping * relativeVel;
        //     glm::vec3 suspensionForce = suspensionAxis * (springForceMag + damperForceMag);

        //     // Apply forces only if the wheel is in contact with the ground.
        //     if (mGroundContact)
        //     {
        //         if (clampedLength == currentLength)
        //         {
        //             // When grounded, apply full suspension forces to both bodies.
        //             //mWheelRb->ApplyForce(suspensionForce, wheelPos);
        //             mCarRb->ApplyForce(-suspensionForce, anchorPos);

        //             mWheelRb->AddForce(suspensionForce);
        //             //mCarRb->AddForce(-suspensionForce);

        //             std::cout << "Suspension force applied: " << suspensionForce.x << ", " << suspensionForce.y << ", " << suspensionForce.z << std::endl;
        //         }
        //         else
        //         {
                 //	std::cout << "Suspension outside of allowed limits." << std::endl;
        //         }
        //     }
        //     else
        //     {
        //         // Not in contact: apply a reduced "recovery" spring force to the wheel only.
        //         // Adjust 'recoveryFactor' as needed (e.g., 0.5 for half stiffness).
        //         float recoveryFactor = 0.5f;
        //         float recoveryForceMag = mStiffness * recoveryFactor * compression;
        //         glm::vec3 recoveryForce = suspensionAxis * recoveryForceMag;
        //         //mWheelRb->ApplyForce(recoveryForce, wheelPos);
                 //mWheelRb->AddForce(recoveryForce);

                 ////std::cout << "Compression : " << compression << std::endl;
        //     }



       //      // Get positions and the suspension axis (local y of the anchor)
       //      std::shared_ptr<Transform> anchorTransform = mAnchorPoint->GetComponent<Transform>();
       //      std::shared_ptr<Transform> wheelTransform = mWheel->GetComponent<Transform>();
       //      glm::vec3 anchorPos = anchorTransform->GetPosition();
       //      glm::vec3 wheelPos = wheelTransform->GetPosition();
       //      glm::vec3 suspensionAxis = glm::normalize(anchorTransform->GetUp());

       //      glm::vec3 distance = wheelPos - anchorPos;
             //float length = glm::length(distance);
       //      if (mGroundContact)
       //      {
       //          if (length < mMaxLength && length > mMinLength)
             //	{
       //              // Normal running, wheel down car up
             //		mWheelRb->AddForce(-mStiffness * suspensionAxis);
       //              mCarRb->ApplyForce(mStiffness * suspensionAxis, anchorPos);
             //	}
       //          else if (length > mMaxLength)
       //          {
       //              // Wheel over max length, wheel up
       //              mWheelRb->AddForce(mStiffness * suspensionAxis);
       //          }
       //      }
       //      else
       //      {
             //	if (length < mMaxLength && length > mMinLength)
             //	{
             //		// Wheel within limts but not touching ground, do nothing
             //	}
             //	else if (length > mMaxLength)
       //          {
             //		// Wheel over max length, wheel up
       //              mWheelRb->AddForce(mStiffness * suspensionAxis);
       //          }
       //          else if (length < mMinLength)
       //          {
       //              // Wheel under min length, wheel down
       //              mWheelRb->AddForce(-mStiffness * suspensionAxis);
       //          }
       //      }


  //      // Get transforms for both anchor points.
  //      std::shared_ptr<Transform> anchorTransform = mAnchorPoint->GetComponent<Transform>();
  //      std::shared_ptr<Transform> wheelTransform = mWheel->GetComponent<Transform>();
  //      glm::vec3 anchorPos = anchorTransform->GetPosition();
  //      glm::vec3 wheelPos = wheelTransform->GetPosition();

  //      glm::vec3 springDir = glm::normalize(anchorTransform->GetUp());

  //      // Calculate the current length along the spring's allowed direction.
  //      float currentLength = glm::dot((wheelPos - anchorPos), springDir);
  //      float clampedLength = glm::clamp(currentLength, mMinLength, mMaxLength);

  //      // Determine displacement from the spring's rest length.
  //      // A negative displacement indicates compression (allowed force), while a positive displacement would be extension.
  //      float displacement = currentLength - mRestLength;

  //      glm::vec3 velB = mWheelRb->GetVelocity();

  //      glm::vec3 anchorVelocity = mCarRb->GetVelocityAtPoint(anchorPos);

  //      glm::vec3 anchorRight = anchorTransform->GetRight();     // Local X
  //      glm::vec3 anchorForward = anchorTransform->GetForward(); // Local Z
  //      glm::vec3 anchorUp = anchorTransform->GetUp();           // Local Y

  //      float anchorVelX = glm::dot(anchorVelocity, anchorRight);
  //      float anchorVelZ = glm::dot(anchorVelocity, anchorForward);
  //      float wheelVelY = glm::dot(velB, anchorUp);

  //      glm::vec3 syncedVelocity = anchorRight * anchorVelX + anchorForward * anchorVelZ + anchorUp * wheelVelY;

  //      //mWheelRb->SetVelocity(syncedVelocity);

  //      // Compute the relative velocity along the spring direction.
  //      glm::vec3 velA = mCarRb->GetVelocityAtPoint(anchorPos);
  //      //glm::vec3 velB = mWheelRb->GetVelocity();
  //      float relativeVel = glm::dot(velB - velA, springDir);

  //      // Calculate spring force using Hooke's law and add damping.
  //      float springForceMag = -mStiffness * displacement;

  //      float velA_Y = glm::dot(velA, springDir);
  //      float velB_Y = glm::dot(mWheelRb->GetVelocity(), springDir);
  //      float relativeVelY = velB_Y - velA_Y;
  //      float damperForceMag = -mDamping * relativeVelY;

  //      //float damperForceMag = -mDamping * relativeVel;
  //      float totalForceMag = springForceMag + damperForceMag;
  //      std::cout << "Current Length: " << currentLength << std::endl;

  //      // Force is applied only along the allowed spring direction.
  //      glm::vec3 force = springDir * totalForceMag;

		//std::cout << "NEW SUSPENSION FUNCTION" << std::endl;
		//

		//std::cout << "Wheel velocity: " << velB.x << ", " << velB.y << ", " << velB.z << std::endl;

  //      std::cout << "Displacement: " << displacement << std::endl;
  //      std::cout << "Relative Vel: " << relativeVel << std::endl;
  //      std::cout << "Damping Force: " << damperForceMag << std::endl;
  //      std::cout << "Spring Force: " << springForceMag << std::endl;

  //      // Apply force to the car body and wheel.
  //      mCarRb->ApplyForce(-force, anchorPos);
  //      //mWheelRb->AddForce(force);
  //      std::cout << "Suspension force applied: " << force.x << ", " << force.y << ", " << force.z << std::endl;

        if (mGroundContact)
        {
            std::shared_ptr<Transform> anchorTransform = mAnchorPoint->GetComponent<Transform>();
            glm::vec3 springDir = anchorTransform->GetUp();

			mCarRb->ApplyForce(mStiffness * springDir, anchorTransform->GetPosition());
        }
    }

    void Suspension::OnTick()
    {
		// Sets the position of the wheel to be the same as the anchor point
        glm::vec3 anchorPos = mAnchorPoint->GetComponent<Transform>()->GetPosition();
        glm::vec3 suspensionAxis = glm::normalize(mAnchorPoint->GetComponent<Transform>()->GetUp());
        glm::vec3 offset = mWheel->GetComponent<Transform>()->GetPosition() - anchorPos;
        float currentLength = glm::dot(offset, suspensionAxis);
        glm::vec3 projectedPos = anchorPos + suspensionAxis * currentLength;

        float alpha = 0.5f;
        glm::vec3 correctedPos = glm::mix(mWheel->GetComponent<Transform>()->GetPosition(), projectedPos, alpha);
        mWheel->GetComponent<Transform>()->SetPosition(anchorPos);


        // Sets the rotation of the wheel to be the same as the anchor point
		mWheel->GetComponent<Transform>()->SetRotation(mAnchorPoint->GetComponent<Transform>()->GetWorldRotationEuler());
    }

}