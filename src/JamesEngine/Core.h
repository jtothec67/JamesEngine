#pragma once

#include "Window.h"

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
		static std::shared_ptr<Core> Initialize();

		void Run();
		std::shared_ptr<Entity> AddEntity();

		std::shared_ptr<Window> GetWindow() const { return mWindow; }
		std::shared_ptr<Input> GetInput() const { return mInput; }
		std::shared_ptr<Resources> GetResources() const { return mResources; }

	private:
		std::shared_ptr<Window> mWindow;
		std::shared_ptr<Input> mInput;
		std::shared_ptr<Resources> mResources;
		std::vector<std::shared_ptr<Entity>> mEntities;
		std::weak_ptr<Core> mSelf;
	};

}