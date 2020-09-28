#include "pch.h"

bool PoolTest()
{
	typedef std::tuple<int, float> TestData;
	ff::PoolAllocator<TestData> pool;
	ff::Vector<TestData*, 256> all;

	for (int i = 0; i < 256; i++)
	{
		TestData* data = pool.New();
		all.Push(data);
		*data = TestData(i, (float)i);
	}

	for (int i = 256 - 1; i >= 0; i--)
	{
		assertRetVal(std::get<0>(*all[i]) == i && std::get<1>(*all[i]) == (float)i, false);
	}

	for (int i = 0; i < 256; i++)
	{
		pool.Delete(all[i]);
	}

	all.Clear();

	// now do it all again
	for (int i = 0; i < 256; i++)
	{
		TestData* data = pool.New();
		all.Push(data);
		*data = TestData(i, (float)i);
	}

	for (int i = 256 - 1; i >= 0; i--)
	{
		assertRetVal(std::get<0>(*all[i]) == i && std::get<1>(*all[i]) == (float)i, false);
	}

	for (int i = 0; i < 256; i++)
	{
		pool.Delete(all[i]);
	}

	all.Clear();

	return true;
}
