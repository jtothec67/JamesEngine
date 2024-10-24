#include <memory>

namespace JamesEngine
{

	class Entity;

	class Component
	{
	public:
		virtual void OnInitialize();
		virtual void OnTick();

	private:
		friend class JamesEngine::Entity;

		std::weak_ptr<Entity> mEntity;

		void Tick();
	};
}