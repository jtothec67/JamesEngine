#pragma once

#include <iostream>

#include <string>
#include <memory>
#include <vector>

namespace JamesEngine
{

	class Core;
	class Component;

	class Entity
	{
	public:
		std::shared_ptr<Core> GetCore();

		std::string GetTag() { return mTag; }
		void SetTag(std::string _tag) { mTag = _tag; }

		template<typename T, typename... Args>
		std::shared_ptr<T> AddComponent(Args&&... args)
		{
			std::shared_ptr<T> rtn = std::make_shared<T>(std::forward<Args>(args)...);

			rtn->mEntity = mSelf;
			rtn->OnInitialize();
			mComponents.push_back(rtn);

			return rtn;
		}

		template <typename T>
		std::shared_ptr<T> GetComponent()
		{
			for (size_t i = 0; i < mComponents.size(); ++i)
			{
				std::shared_ptr<T> rtn = std::dynamic_pointer_cast<T>(mComponents[i]);
				if (rtn)
				{
					return rtn;
				}
			}

			return nullptr;
		}

		void Destroy();

	private:
		friend class JamesEngine::Core;

		std::weak_ptr<Core> mCore;
		std::weak_ptr<Entity> mSelf;

		std::vector<std::shared_ptr<Component>> mComponents;

		std::string mTag = "Default";

		bool alive = true;

		void OnTick();
		void OnRender();
		void OnGUI();
	};

}