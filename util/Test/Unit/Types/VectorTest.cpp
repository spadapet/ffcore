#include "pch.h"

typedef std::tuple<int, float> TestData;

template<typename T>
static bool InternalVectorTest()
{
	T vec;

	for(int i = 0; i < 8; i++)
		vec.Push(TestData(i, (float)i));

	for(int i = 0; i < 8; i++)
		vec.Insert(vec.Size(), TestData(8 + i, 8.0f + i));

	assertRetVal(vec.Size() == 16, false);
	assertRetVal(vec.Allocated() == 16, false);

	assertRetVal(*vec.Data() == TestData(0, 0.0f), false);
	assertRetVal(*vec.Data(15) == TestData(15, 15.0f), false);

	T vec2 = vec;
	assertRetVal(vec == vec2, false);

	vec.Delete(4, 4);
	assertRetVal(vec.Size() == 12, false);
	assertRetVal(*vec.Data(4) == TestData(8, 8.0f), false);

	assertRetVal(vec.GetAt(8) == *vec.Data(8), false);

	vec.Insert(4, TestData(4, 4.0f));
	vec.Insert(5, TestData(5, 5.0f));
	vec.Insert(6, TestData(6, 6.0f));
	vec.Insert(7, TestData(7, 7.0f));
	assertRetVal(vec == vec2, false);

	vec.Reserve(32);
	assertRetVal(vec.Allocated() == 32, false);

	vec.Resize(8);
	assertRetVal(vec.Allocated() == 32 && vec.Size() == 8, false);

	vec.Clear();
	assertRetVal(!vec.Size(), false);

	return true;
}

bool VectorTest()
{
	bool success = InternalVectorTest<typename ff::Vector<TestData, 0>>() &&
		InternalVectorTest<typename ff::Vector<TestData, 2>>() &&
		InternalVectorTest<typename ff::Vector<TestData, 8>>();
	assertRetVal(success, false);

	return true;
}
