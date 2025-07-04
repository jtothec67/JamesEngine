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

struct ShadowCascade
{
	glm::vec2 orthoSize;
	glm::ivec2 resolution;
	float nearPlane;
	float farPlane;
	float splitDepthStart;
	float splitDepthEnd;
	std::shared_ptr<Renderer::RenderTexture> renderTexture;
	glm::mat4 lightSpaceMatrix;
};

class LightManager
{
public:
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

	void AddShadowCascade(glm::vec2 orthoSize, glm::ivec2 resolution, float nearPlane, float farPlane, float splitDepthStart, float splitDepthEnd)
	{
		ShadowCascade cascade;
		cascade.orthoSize = orthoSize;
		cascade.resolution = resolution;
		cascade.nearPlane = nearPlane;
		cascade.farPlane = farPlane;
		cascade.splitDepthStart = splitDepthStart;
		cascade.splitDepthEnd = splitDepthEnd;
		cascade.renderTexture = std::make_shared<Renderer::RenderTexture>(resolution.x, resolution.y, Renderer::RenderTextureType::Depth);
		mCascades.push_back(cascade);
	}

	void ClearShadowCascades()
	{
		mCascades.clear();
	}

	const std::vector<ShadowCascade>& GetShadowCascades() const { return mCascades; }
	std::vector<ShadowCascade>& GetShadowCascades() { return mCascades; } // Allow non-const access for updating matrices

	void SetupDefault3Cascades()
	{
		ClearShadowCascades();
		AddShadowCascade({ 10.f, 10.f }, { 3500, 3500 }, 0.1f, 100.0f, 0, 30);
		AddShadowCascade({ 60.f, 60.f }, { 3499, 3499 }, 0.1f, 100.0f, 30, 60);
		AddShadowCascade({ 150.f, 150.f }, { 3498, 3498 }, 0.1f, 100.0f, 60, 120);
	}


private:
	std::vector<std::shared_ptr<Light>> mLights;
	glm::vec3 mAmbient = glm::vec3(1.f, 1.f, 1.f);

	glm::vec3 mDirectionalLightDirection = glm::vec3(0.f, -1.f, 0.f);
	glm::vec3 mDirectionalLightColour = glm::vec3(1.f, 1.f, 1.f);

	std::vector<ShadowCascade> mCascades;
};