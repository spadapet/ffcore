#pragma once

#include "Value/Value.h"

namespace ff
{
	class Dict;
	class IDataReader;
	class IDataWriter;
	class Log;

	UTIL_API bool SaveTypedValue(ff::IDataWriter* stream, const ff::Value* value);
	UTIL_API ff::ValuePtr LoadTypedValue(ff::IDataReader* stream);
	UTIL_API void DumpValue(ff::StringRef name, const Value* value, ff::Log* log, bool debugOnly);
	UTIL_API void DebugDumpValue(const Value* value);
}
