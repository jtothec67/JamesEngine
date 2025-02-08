#pragma once

#include "Window.h"
#include "Audio.h"
#include "GUI.h"
#include "LightManager.h"

#include <memory>
#include <vector>

namespace JamesEngine
{

	class Input;
	class Entity;
	class Resources;
	class Camera;
	class Skybox;

	/**
	 * @class Core
	 * @brief The main class for initializing and running the engine.
	 */
	class Core
	{
	public:
		/**
		 * @brief Initializes the Core with a given window size.
		 * @param _windowSize The initial size of the window.
		 * @return A shared pointer to the initialized Core.
		 */
		static std::shared_ptr<Core> Initialize(glm::ivec2 _windowSize);

		/**
		 * @brief Runs the main loop of the engine.
		 */
		void Run();
		/**
		 * @brief Stops the execution of the engine.
		 */
		void End() { mIsRunning = false; }

		std::shared_ptr<Window> GetWindow() const { return mWindow; }
		std::shared_ptr<Input> GetInput() const { return mInput; }
		std::shared_ptr<Resources> GetResources() const { return mResources; }
		std::shared_ptr<GUI> GetGUI() const { return mGUI; }
		std::shared_ptr<LightManager> GetLightManager() const { return mLightManager; }
		std::shared_ptr<Skybox> GetSkybox() const { return mSkybox; }

		/**
		 * @brief Adds a new entity to the engine.
		 * @return A shared pointer to the newly added entity.
		 */
		std::shared_ptr<Entity> AddEntity();

		/**
		 * @brief Gets the delta time.
		 * @return The time taken for the last frame to update and draw.
		 */
		float DeltaTime() { return mDeltaTime; }

		/**
		 * @brief Gets the current camera with the highest priority.
		 * @return A shared pointer to the camera.
		 */
		std::shared_ptr<Camera> GetCamera();

		/**
		 * @brief Destroys all entities in the scene. (Make sure to add entities after or the engine sof tlocks itself.)
		 */
		void DestroyAllEntities();

		/**
		 * @brief Finds all entities with tag _tag in the entities.
		 * @param _tag A tag to identify entities.
		 * @return A vector of shared pointers to the found entities.
		 */
		std::vector<std::shared_ptr<Entity>> GetEntitiesByTag(std::string _tag);

		/**
		* @brief Finds the first entity with tag _tag in the entities. Useful if you know there is only one.
		* @param _tag A tag to identify entities.
		* @return A shared pointer to the found entity, or nullptr if not found.
		*/
		std::shared_ptr<Entity> GetEntityByTag(std::string _tag);

		/**
		 * @brief Finds all components of type T in the entities.
		 * @tparam T The type of components to find.
		 * @param _out A vector to store the found components.
		 */
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

		/**
		 * @brief Finds the first component of type T in the entities. Useful if you know there is only one.
		 * @tparam T The type of component to find.
		 * @return A shared pointer to the found component, or nullptr if not found.
		 */
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
		std::shared_ptr<LightManager> mLightManager;
		std::shared_ptr<Skybox> mSkybox;
		std::shared_ptr<Resources> mResources;
		std::vector<std::shared_ptr<Entity>> mEntities;
		std::weak_ptr<Core> mSelf;

		float mDeltaTime = 0.0f;

		bool mIsRunning = true;

		/*glm::vec3 mLightPos = glm::vec3(0.f, 20.f, 0.f);
		glm::vec3 mLightColor = glm::vec3(1.f, 1.f, 1.f);
		glm::vec3 mAmbient = glm::vec3(0.1f, 0.1f, 0.1f);
		float mLightStrength = 1.f;*/

		// Used when loading scenes to ensure first frame doesn't have a large delta time
		bool mDeltaTimeZero = false;
	};

}