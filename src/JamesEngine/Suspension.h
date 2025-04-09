#pragma once

#include "Component.h"

#ifdef _DEBUG
#include "Renderer/Shader.h"
#endif


namespace JamesEngine
{
	class Rigidbody;

	class Suspension : public Component
	{
	public:
#ifdef _DEBUG
		void OnGUI();
#endif

		void OnTick();

		void SetWheel(std::shared_ptr<Entity> _wheel) { mWheel = _wheel; }
		void SetCarBody(std::shared_ptr<Entity> _carBody) { mCarBody = _carBody; }
		void SetAnchorPoint(std::shared_ptr<Entity> _anchorPoint) { mAnchorPoint = _anchorPoint; }

		void SetRestLength(float _restLength) { mRestLength = _restLength; }
		void SetStiffness(float _stiffness) { mStiffness = _stiffness; }
		void SetDamping(float _damping) { mDamping = _damping; }
		void SetMinLength(float _minLength) { mMinLength = _minLength; }
		void SetMaxLength(float _maxLength) { mMaxLength = _maxLength; }

		void SetDebugVisual(bool _value) { mDebugVisual = _value; }

	private:
		std::shared_ptr<Entity> mWheel;
		std::shared_ptr<Entity> mCarBody;
		std::shared_ptr<Entity> mAnchorPoint;

		std::shared_ptr<Rigidbody> mWheelRb;
		std::shared_ptr<Rigidbody> mCarRb;

		float mStiffness = 30000;
		float mDamping = 100;

		float mRestLength = 0.2f;

		float mMinLength = 0.0f;
		float mMaxLength = 0.4f;

		bool mDebugVisual = true;

#ifdef _DEBUG
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../assets/shaders/OutlineShader.vert", "../assets/shaders/OutlineShader.frag");
		std::shared_ptr<Renderer::Model> mModel = std::make_shared<Renderer::Model>("../assets/shapes/spring.obj");
#endif
	};

}