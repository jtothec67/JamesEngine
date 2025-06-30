#pragma once

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

	void SetDirectionalLightDirection(glm::vec3 _direction) { mDirectionalLightDirection = glm::normalize(_direction); }
	glm::vec3 GetDirectionalLightDirection() { return mDirectionalLightDirection; }

	void SetDirectionalLightColour(glm::vec3 _colour) { mDirectionalLightColour = _colour; }
	glm::vec3 GetDirectionalLightColour() { return mDirectionalLightColour; }

	void SetAmbient(glm::vec3 _ambient) { mAmbient = _ambient; }
	glm::vec3 GetAmbient() { return mAmbient; }

private:
	std::vector<std::shared_ptr<Light>> mLights;

	glm::vec3 mDirectionalLightDirection = glm::vec3(0.f, -1.f, 0.f);
	glm::vec3 mDirectionalLightColour = glm::vec3(1.f, 1.f, 1.f);

	glm::vec3 mAmbient = glm::vec3(1.f, 1.f, 1.f);
};