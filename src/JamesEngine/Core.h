#include <memory>
#include <vector>

namespace JamesEngine
{

	class Entity;

	class Core
	{
	public:
		static std::shared_ptr<Core> Initialize();

		void Start();
		std::shared_ptr<Entity> AddEntity();
	private:
		std::weak_ptr<Core> mSelf;
		std::vector<std::shared_ptr<Entity>> mEntities;
	};

}