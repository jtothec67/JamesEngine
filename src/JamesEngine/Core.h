#pragma once

#include "Window.h"
#include "Audio.h"
#include "GUI.h"

#include <memory>
#include <vector>

namespace JamesEngine
{

	class Input;
	class Entity;
	class Resources;
	class Camera;
	class Skybox;

	class Core
	{
	public:
		static std::shared_ptr<Core> Initialize(glm::ivec2 _windowSize);

		void Run();
		std::shared_ptr<Entity> AddEntity();

		std::shared_ptr<Window> GetWindow() const { return mWindow; }
		std::shared_ptr<Input> GetInput() const { return mInput; }
		std::shared_ptr<Resources> GetResources() const { return mResources; }
		std::shared_ptr<GUI> GetGUI() const { return mGUI; }
		std::shared_ptr<Skybox> GetSkybox() const { return mSkybox; }

		void SetLightPosition(glm::vec3 _pos) { mLightPos = _pos; }
		glm::vec3 GetLightPosition() { return mLightPos; }

		void SetLightColor(glm::vec3 _color) { mLightColor = _color; }
		glm::vec3 GetLightColor() { return mLightColor; }

		void SetAmbient(glm::vec3 _ambient) { mAmbient = _ambient; }
		glm::vec3 GetAmbient() { return mAmbient; }

		void SetLightStrength(float _strength) { mLightStrength = _strength; }
		float GetLightStrength() { return mLightStrength; }

		float DeltaTime() { return mDeltaTime; }

		std::shared_ptr<Camera> GetCamera();

		// Returns all components of type T found in the entities
		template <typename T>
		void FindComponents(std::vector<std::shared_ptr<T>>& _out)
		{
			for (size_t ei = 0; ei < mEntities.size(); ++ei)
			{
				std::shared_ptr<Entity> e = mEntities.at(ei);
				for (size_t ci = 0; ci < e->mComponents.size(); ++ci)
				{
					std::shared_ptr<Component> c = e->mComponents.at(ci);
					std::shared_ptr<T> t = std::dynamic_pointer_cast<T>(c);

					if (t)
					{
						_out.push_back(t);
					}
				}
			}
		}

		// Returns the first component of type T found in the entities (useful if you know only one exists)
		template <typename T>
		std::shared_ptr<T> FindComponent()
		{
			for (size_t ei = 0; ei < mEntities.size(); ++ei)
			{
				std::shared_ptr<Entity> e = mEntities.at(ei);
				for (size_t ci = 0; ci < e->mComponents.size(); ++ci)
				{
					std::shared_ptr<Component> c = e->mComponents.at(ci);
					std::shared_ptr<T> t = std::dynamic_pointer_cast<T>(c);

					if (t)
					{
						return t;
					}
				}
			}

			return nullptr;
		}

	private:
		std::shared_ptr<Window> mWindow;
		std::shared_ptr<Audio> mAudio;
		std::shared_ptr<Input> mInput;
		std::shared_ptr<GUI> mGUI;
		std::shared_ptr<Skybox> mSkybox;
		std::shared_ptr<Resources> mResources;
		std::vector<std::shared_ptr<Entity>> mEntities;
		std::weak_ptr<Core> mSelf;

		float mDeltaTime = 0.0f;

		bool mIsRunning = true;

		glm::vec3 mLightPos = glm::vec3(0.f, 20.f, 0.f);
		glm::vec3 mLightColor = glm::vec3(1.f, 1.f, 1.f);
		glm::vec3 mAmbient = glm::vec3(0.1f, 0.1f, 0.1f);
		float mLightStrength = 1.f;
	};

}