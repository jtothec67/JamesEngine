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
	glm::ivec2 resolution;
	glm::vec2 splitDepths;
	std::shared_ptr<Renderer::RenderTexture> renderTexture;
	glm::mat4 lightSpaceMatrix;
};

struct PreBakedShadowMap
{
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

	void AddShadowCascade(glm::ivec2 resolution, glm::vec2 splitDepths)
	{
		ShadowCascade cascade;
		cascade.resolution = resolution;
		cascade.splitDepths = splitDepths;
		cascade.renderTexture = std::make_shared<Renderer::RenderTexture>(resolution.x, resolution.y, Renderer::RenderTextureType::Depth);
		mCascades.push_back(cascade);
	}

	void AddPreBakedShadowMap(std::shared_ptr<Renderer::RenderTexture> _shadowMap, glm::mat4 _lightSpaceMatrix)
	{
		PreBakedShadowMap preBaked;
		preBaked.renderTexture = _shadowMap;
		preBaked.lightSpaceMatrix = _lightSpaceMatrix;
		mPreBakedShadowMaps.push_back(preBaked);
	}

	void ClearShadowCascades()
	{
		mCascades.clear();
	}

	void ClearPreBakedShadowMaps()
	{
		mPreBakedShadowMaps.clear();
	}

	const std::vector<ShadowCascade>& GetShadowCascades() const { return mCascades; }
	std::vector<ShadowCascade>& GetShadowCascades() { return mCascades; }

	const std::vector<PreBakedShadowMap>& GetPreBakedShadowMaps() const { return mPreBakedShadowMaps; }
	std::vector<PreBakedShadowMap>& GetPreBakedShadowMaps() { return mPreBakedShadowMaps; }

	void SetupDefault3Cascades()
	{
		ClearShadowCascades();
		AddShadowCascade({ 4048, 4048 }, { 0, 20 });
		AddShadowCascade({ 2024, 2024 }, { 20, 60 });
		AddShadowCascade({ 2024, 2024 }, { 60, 200 });
	}

	std::shared_ptr<std::vector<std::shared_ptr<Renderer::RenderTexture>>> GetShadowMaps()
	{
		std::shared_ptr <std::vector<std::shared_ptr<Renderer::RenderTexture>>> shadowMaps = std::make_shared<std::vector<std::shared_ptr<Renderer::RenderTexture>>>(); // yuck

		shadowMaps->reserve(mCascades.size() + mPreBakedShadowMaps.size());
		for (const auto& cascade : mCascades)
		{
			shadowMaps->emplace_back(cascade.renderTexture);
		}
		for (const auto& preBaked : mPreBakedShadowMaps)
		{
			shadowMaps->emplace_back(preBaked.renderTexture);
		}
		return shadowMaps;
	}

	std::shared_ptr <std::vector<glm::mat4>> GetLightSpaceMatrices()
	{
		std::shared_ptr <std::vector<glm::mat4>> matrices = std::make_shared<std::vector<glm::mat4>>();
		matrices->reserve(mCascades.size() + mPreBakedShadowMaps.size());
		for (const auto& cascade : mCascades)
		{
			matrices->emplace_back(cascade.lightSpaceMatrix);
		}
		for (const auto& preBaked : mPreBakedShadowMaps)
		{
			matrices->emplace_back(preBaked.lightSpaceMatrix);
		}
		return matrices;
	}

	bool ArePreBakedShadowsUploaded() const { return mPreBakedShadowsUploaded; }
	void SetPreBakedShadowsUploaded(bool uploaded) { mPreBakedShadowsUploaded = uploaded; }

private:
	std::vector<std::shared_ptr<Light>> mLights;
	glm::vec3 mAmbient = glm::vec3(1.f, 1.f, 1.f);

	glm::vec3 mDirectionalLightDirection = glm::vec3(0.f, -1.f, 0.f);
	glm::vec3 mDirectionalLightColour = glm::vec3(1.f, 1.f, 1.f);

	std::vector<ShadowCascade> mCascades;

	std::vector<PreBakedShadowMap> mPreBakedShadowMaps;

	bool mPreBakedShadowsUploaded = false;
};