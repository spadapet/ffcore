#include "pch.h"

bool ListTest()
{
	ff::List<int> list;

	assertRetVal(list.Size() == 0, false);
	assertRetVal(list.GetFirst() == list.GetLast(), false);
	assertRetVal(list.begin() == list.end(), false);
	assertRetVal(list.cbegin() == list.cend(), false);

	for (int i = 0; i < 100; i++)
	{
		list.Push(i);
	}

	assertRetVal(list.Size() == 100, false);
	assertRetVal(list.GetFirst() && *list.GetFirst() == 0, false);
	assertRetVal(list.GetLast() && *list.GetLast() == 99, false);

	int count = 0;
	for (int iter: list)
	{
		assertRetVal(iter == count++, false);
	}

	count = 99;
	for (auto iter = list.crbegin(); iter != list.crend(); iter++)
	{
		assertRetVal(*iter == count--, false);
	}

	return true;
}
