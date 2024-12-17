#pragma once

#include "Window.h"
#include "Audio.h"

#include <memory>
#include <vector>

namespace JamesEngine
{

	class Input;
	class Entity;
	class Resources;

	class Core
	{
	public:
		static std::shared_ptr<Core> Initialize(glm::ivec2 _windowSize);

		void Run();
		std::shared_ptr<Entity> AddEntity();

		std::shared_ptr<Window> GetWindow() const { return mWindow; }
		std::shared_ptr<Input> GetInput() const { return mInput; }
		std::shared_ptr<Resources> GetResources() const { return mResources; }

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

	private:
		std::shared_ptr<Window> mWindow;
		std::shared_ptr<Audio> mAudio;
		std::shared_ptr<Input> mInput;
		std::shared_ptr<Resources> mResources;
		std::vector<std::shared_ptr<Entity>> mEntities;
		std::weak_ptr<Core> mSelf;
	};

}