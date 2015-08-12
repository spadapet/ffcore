#pragma once

namespace ff
{
	class Dict;
	class Log;
	class IData;
	class IDataReader;
	class IDataWriter;
	class Value;

	UTIL_API bool SaveDict(const Dict& dict, ff::IDataWriter* writer);
	UTIL_API ff::ComPtr<ff::IData> SaveDict(const Dict& dict);
	UTIL_API bool LoadDict(ff::IDataReader* reader, Dict& dict);
	UTIL_API void DumpDict(ff::StringRef name, const Dict& dict, ff::Log* log, bool debugOnly);
	UTIL_API void DebugDumpDict(const Dict& dict);
}
