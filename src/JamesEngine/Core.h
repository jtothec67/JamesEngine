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
	private:
		std::weak_ptr<Core> mSelf;
		std::vector<std::shared_ptr<Entity>> mEntities;

		std::shared_ptr<Window> mWindow;
	};

}