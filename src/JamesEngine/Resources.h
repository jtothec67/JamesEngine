#pragma once

#include "Resource.h"

#include <vector>
#include <memory>

namespace JameEngine
{

	class Resource;

	class Resources
	{
	public:
		template <typename T>
		std::shared_ptr<T> load(const std::stringbuf& _path)
		{
			for (size_t i = 0; i < m_resources.size(); ++i)
			{
				if (m_resources.at(i)->getPath() == _path)
				{
					return m_resources.at(i);
				}
			}
			// Create new instance, construct it and add to cache
			std::shared_ptr<T> rtn = std::make_shared<T>();
			rtn->m_path = _path;
			rtn->load();
			m_resources.push_back(rtn);
			return rtn;
		}

	private:
		std::vector<std::shared_ptr<Resource>> mResources;
	};

}