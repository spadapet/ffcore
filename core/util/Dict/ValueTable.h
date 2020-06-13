#pragma once
#include "Value/Value.h"

namespace ff
{
	class Dict;
	class Value;

	class IValueAccess
	{
	public:
		virtual ValuePtr GetValue(StringRef name) const = 0;
		virtual String GetString(StringRef name) const = 0;
	};

	// Read-only interface to a Dict that can be persisted as a resource
	class __declspec(uuid("4e7f1faf-85eb-47ef-b78d-ab17b9366ea2")) __declspec(novtable)
		IValueTable : public IUnknown, public IValueAccess
	{
	};

	UTIL_API bool CreateValueTable(const Dict& dict, IValueTable** obj);
}
