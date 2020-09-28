#include "pch.h"
#include "Dict/Dict.h"
#include "Dict/SmallDict.h"
#include "Globals/Log.h"
#include "Globals/ProcessGlobals.h"
#include "Types/Timer.h"
#include "Value/Values.h"

static bool RunDictPerfCompare(size_t entryCount)
{
	ff::Dict dict;
	ff::SmallDict smallDict1;
	ff::SmallDict smallDict2;
	ff::Vector<ff::String> keys;
	ff::Vector<ff::ValuePtr> values;

	keys.Reserve(entryCount);
	values.Reserve(entryCount);

	smallDict1.Reserve(entryCount);
	smallDict2.Reserve(entryCount);

	for (size_t i = 0; i < entryCount; i++)
	{
		ff::String key = ff::String::format_new(L"%lu-%lu-%lu-%lu", i, i, i, i);
		keys.Push(key);

		ff::ValuePtr value = ff::Value::New<ff::DoubleValue>((double)i);
		values.Push(value);

		ff::ProcessGlobals::Get()->GetStringCache().CacheString(key);
	}

	ff::Timer timer;

	for (size_t i = 0; i < entryCount; i++)
	{
		dict.SetValue(keys[i], values[i]);
	}

	double dictTime = timer.Tick();

	for (size_t i = 0; i < entryCount; i++)
	{
		smallDict1.Add(keys[i], values[i]);
	}

	double smallDictTime1 = timer.Tick();

	if (entryCount <= 100000)
	{
		for (size_t i = 0; i < entryCount; i++)
		{
			smallDict2.Set(keys[i], values[i]);
		}
	}

	double smallDictTime2 = timer.Tick();

	ff::String status = ff::String::format_new(
		L"Dict add with %lu entries: DictSet:%fs, SmallDictAdd:%fs, SmallDictSet:%fs\r\n",
		entryCount,
		dictTime,
		smallDictTime1,
		smallDictTime2);
	ff::Log::DebugTraceF(status.c_str());
	std::wcout << status.c_str();

	timer.Reset();

	for (size_t i = 0; i < entryCount; i++)
	{
		ff::ValuePtr value = dict.GetValue(keys[i]);
	}

	dictTime = timer.Tick();

	for (size_t i = 0; i < entryCount; i++)
	{
		ff::ValuePtr value = smallDict1.GetValue(keys[i]);
	}

	smallDictTime1 = timer.Tick();

	status = ff::String::format_new(
		L"Dict get with %lu entries: DictGet:%fs, SmallDictGet:%fs\r\n",
		entryCount,
		dictTime,
		smallDictTime1);
	ff::Log::DebugTraceF(status.c_str());
	std::wcout << status.c_str();

	std::wcout << L"\r\n";

	return true;
}

bool DictPerfTest()
{
	assertRetVal(RunDictPerfCompare(1), false);
	assertRetVal(RunDictPerfCompare(10), false);
	assertRetVal(RunDictPerfCompare(100), false);
	assertRetVal(RunDictPerfCompare(500), false);
	assertRetVal(RunDictPerfCompare(750), false);
	assertRetVal(RunDictPerfCompare(1000), false);
	assertRetVal(RunDictPerfCompare(2500), false);
	assertRetVal(RunDictPerfCompare(5000), false);
	assertRetVal(RunDictPerfCompare(10000), false);
	assertRetVal(RunDictPerfCompare(50000), false);
	assertRetVal(RunDictPerfCompare(100000), false);

	return true;
}
