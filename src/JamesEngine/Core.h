#pragma once

#include <memory>
#include <vector>

namespace JamesEngine
{

	class Entity;
	class Window;

	class Core
	{
	public:
		static std::shared_ptr<Core> Initialize();

		void Run();
		std::shared_ptr<Entity> AddEntity();
		std::shared_ptr<Window> GetWindow() const { return mWindow; }
	private:
		std::shared_ptr<Window> mWindow;
		std::vector<std::shared_ptr<Entity>> mEntities;
		std::weak_ptr<Core> mSelf;
	};

}