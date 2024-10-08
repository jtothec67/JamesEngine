#include <memory>

namespace JamesEngine
{

	class Core;
	class Component;

	class Entity
	{
	public:
		template <typename T>
		std::shared_ptr<T> AddComponent()
		{
			std::shared_ptr<T> rtn = std::make_shared<T>();

			mComponents.push_back(rtn);

			return rtn;
		}
	private:
		friend class JamesEngine::Core;

		std::weak_ptr<Core> mCore;

		std::vector<std::shared_ptr<Component>> mComponents;
	};

}