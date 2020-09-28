#pragma once

#include "Value/Value.h"

namespace ff
{
	class StringValue : public ff::Value
	{
	public:
		UTIL_API StringValue(ff::String&& value);
		UTIL_API StringValue(ff::StringRef value);

		UTIL_API ff::StringRef GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(ff::String&& value);
		UTIL_API static ff::Value* GetStaticValue(ff::StringRef value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::String _value;
	};
}
