#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>

namespace JamesEngine
{

	class Resource;

	class Resources
	{
	public:
		template <typename T>
		std::shared_ptr<T> Load(const std::string& _path)
		{
			for (size_t i = 0; i < mResources.size(); ++i)
			{
				if (mResources.at(i)->GetPath() == "../assets/" + _path)
				{
					std::cout << "Resource already loaded: " << _path << std::endl;
					return std::dynamic_pointer_cast<T>(mResources.at(i));
				}
			}

			std::shared_ptr<T> rtn = std::make_shared<T>();
			rtn->SetPath("../assets/" + _path);
			rtn->OnLoad();
			mResources.push_back(rtn);
			std::cout << "Resource loaded: " << _path << std::endl;
			return rtn;
		}

	private:
		std::vector<std::shared_ptr<Resource>> mResources;
	};

}