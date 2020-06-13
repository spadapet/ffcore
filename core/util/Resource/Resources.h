#pragma once

#include "Resource/ResourceValue.h"

namespace ff
{
	class AppGlobals;
	class Dict;

	class IResourceAccess
	{
	public:
		virtual SharedResourceValue GetResource(StringRef name) = 0;
		virtual Vector<String> GetResourceNames() const = 0;
	};

	class __declspec(uuid("1894118b-40da-4055-8ffe-e28688a61a3f")) __declspec(novtable)
		IResources : public IUnknown, public IResourceAccess
	{
	public:
		virtual void AddResources(const Dict& dict) = 0;
		virtual SharedResourceValue FlushResource(SharedResourceValue value) = 0;
	};

	UTIL_API bool CreateResources(AppGlobals* globals, const Dict& dict, IResources** obj);
}