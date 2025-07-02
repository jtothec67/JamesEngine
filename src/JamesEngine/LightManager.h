#pragma once

#include "Renderer/RenderTexture.h"

#include <glm/glm.hpp>
#include <GL/glew.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>

struct Light
{
	std::string name;
	glm::vec3 position;
	glm::vec3 colour;
	float strength;
};

class LightManager
{
public:
	void InitShadowMap()
	{
		mDirectionalLightShadowMap = std::make_unique<Renderer::RenderTexture>(mShadowMapSize.x, mShadowMapSize.y, Renderer::RenderTextureType::Depth);
	}

	std::vector<std::shared_ptr<Light>> GetLights() { return mLights; }

	void AddLight(std::string _name, glm::vec3 _position, glm::vec3 _colour, float _strength)
	{
		std::shared_ptr<Light> l = std::make_shared<Light>();
		l->position = _position;
		l->colour = _colour;
		l->strength = _strength;
		mLights.push_back(l);
	}

	void RemoveLight(std::string _name)
	{
		for (size_t i = 0; i < mLights.size(); ++i)
		{
			if (mLights.at(i)->name == _name)
			{
				mLights.erase(mLights.begin() + i);
				return;
			}

			std::cout << "Light " << _name << " not found." << std::endl;
		}
	}

	void SetAmbient(glm::vec3 _ambient) { mAmbient = _ambient; }
	glm::vec3 GetAmbient() { return mAmbient; }

	void SetDirectionalLightDirection(glm::vec3 _direction) { mDirectionalLightDirection = glm::normalize(_direction); }
	glm::vec3 GetDirectionalLightDirection() { return mDirectionalLightDirection; }

	void SetDirectionalLightColour(glm::vec3 _colour) { mDirectionalLightColour = _colour; }
	glm::vec3 GetDirectionalLightColour() { return mDirectionalLightColour; }

	void SetOrthoShadowMapSize(glm::vec2 _size) { mOrthoShadowMapSize = _size; }
	glm::vec2 GetOrthoShadowMapSize() { return mOrthoShadowMapSize; }

	void SetDirectionalLightNearPlane(float _nearPlane) { mDirectionalLightNearPlane = _nearPlane; }
	float GetDirectionalLightNearPlane() { return mDirectionalLightNearPlane; }

	void SetDirectionalLightFarPlane(float _farPlane) { mDirectionalLightFarPlane = _farPlane; }
	float GetDirectionalLightFarPlane() { return mDirectionalLightFarPlane; }

	Renderer::RenderTexture* GetDirectionalLightShadowMap() { return mDirectionalLightShadowMap.get(); }

private:
	std::vector<std::shared_ptr<Light>> mLights;

	glm::vec3 mAmbient = glm::vec3(1.f, 1.f, 1.f);

	glm::vec3 mDirectionalLightDirection = glm::vec3(0.f, -1.f, 0.f);
	glm::vec3 mDirectionalLightColour = glm::vec3(1.f, 1.f, 1.f);
	glm::vec2 mOrthoShadowMapSize = glm::vec2(100.f, 100.f);
	glm::ivec2 mShadowMapSize = glm::ivec2(10000, 10000);
	float mDirectionalLightNearPlane = 1.f;
	float mDirectionalLightFarPlane = 100.f;

	std::unique_ptr<Renderer::RenderTexture> mDirectionalLightShadowMap;
};