#pragma once

#include "Component.h"

#include <fcl/fcl.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp> 

#ifdef _DEBUG
#include "Renderer/Shader.h"
#endif


namespace JamesEngine
{

	class Collider : public Component
	{
	public:
#ifdef _DEBUG
		virtual void OnGUI() {}
#endif

		virtual bool IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth) = 0;

		virtual glm::mat3 UpdateInertiaTensor(float _mass) = 0;

		virtual fcl::CollisionObjectd* GetFCLObject() { return mFCLObject.get(); }

		void UpdateFCLTransform(const glm::vec3& worldPosition, const glm::quat& worldRotation)
		{
			// Convert rotation offset from degrees to radians, then to quaternion
			glm::quat rotationOffsetQuat = glm::quat(glm::radians(mRotationOffset));

			// Combine world rotation with local rotation offset
			glm::quat finalRotation = worldRotation * rotationOffsetQuat;

			// Rotate the position offset into world space
			glm::vec3 finalPosition = worldPosition + worldRotation * mPositionOffset;

			// Convert to FCL transform
			Eigen::Quaterniond eigenQuat(finalRotation.w, finalRotation.x, finalRotation.y, finalRotation.z);
			fcl::Matrix3d rotationMatrix = eigenQuat.toRotationMatrix();
			fcl::Vector3d translation(finalPosition.x, finalPosition.y, finalPosition.z);

			fcl::Transform3d fclTransform;
			fclTransform.linear() = rotationMatrix;
			fclTransform.translation() = translation;

			if (mFCLObject)
			{
				mFCLObject->setTransform(fclTransform);
				mFCLObject->computeAABB();
			}
		}

		void SetPositionOffset(glm::vec3 _offset) { mPositionOffset = _offset; }
		glm::vec3 GetPositionOffset() { return mPositionOffset; }

		void SetRotationOffset(glm::vec3 _rotation) { mRotationOffset = _rotation; }
		glm::vec3 GetRotationOffset() { return mRotationOffset; }

		void SetDebugVisual(bool _value) { mDebugVisual = _value; }

	protected:
		friend class BoxCollider;
		friend class SphereCollider;

		void InitFCLObject(std::shared_ptr<fcl::CollisionGeometryd> shape)
		{
			mFCLObject = std::make_shared<fcl::CollisionObjectd>(shape);
		}

		std::shared_ptr<fcl::CollisionObjectd> mFCLObject;

		glm::vec3 mPositionOffset{ 0 };
		glm::vec3 mRotationOffset{ 0 };

		bool mDebugVisual = true;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../assets/shaders/OutlineShader.vert", "../assets/shaders/OutlineShader.frag");
#endif
	};

}