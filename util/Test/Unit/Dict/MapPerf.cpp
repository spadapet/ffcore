#include "pch.h"
#include "Dict/Dict.h"
#include "Globals/Log.h"
#include "Types/Timer.h"

#include <unordered_map>
#include <string>

static bool RunMapPerfCompare(size_t entryCount)
{
	ff::Vector<ff::String> keys;
	ff::Vector<std::wstring> keys2;
	ff::Vector<size_t> values;
	ff::Map<ff::String, size_t> map;
	std::unordered_map<std::wstring, size_t> map2;

	keys.Reserve(entryCount);
	keys2.Reserve(entryCount);
	values.Reserve(entryCount);

	for (size_t i = 0; i < entryCount; i++)
	{
		ff::String key = ff::String::format_new(L"%lu-%lu-%lu-%lu", i, i, i, i);
		keys.Push(key);
		keys2.Push(std::wstring(key.c_str()));
		values.Push(i);
	}

	ff::Timer timer;

	for (size_t i = 0; i < 8; i++)
	{
		map.Clear();

		for (size_t i = 0; i < entryCount; i++)
		{
			map.SetKey(keys[i], values[i]);
		}
	}

	double myTime = timer.Tick();

	for (size_t i = 0; i < 8; i++)
	{
		map2.clear();

		for (size_t i = 0; i < entryCount; i++)
		{
			map2.emplace(keys2[i], values[i]);
		}
	}

	double crtTime = timer.Tick();

	ff::String status = ff::String::format_new(
		L"Map with %lu entries, 8 times: ff::Map:%fs, std::unordered_map:%fs\r\n",
		entryCount,
		myTime,
		crtTime);
	ff::Log::DebugTraceF(status.c_str());
	std::wcout << status.c_str();

	return true;
}

bool MapPerfTest()
{
	::RunMapPerfCompare(100);
	::RunMapPerfCompare(10000);
	::RunMapPerfCompare(1000000);

	return true;
}
