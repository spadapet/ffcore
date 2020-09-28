#include "pch.h"

bool CompareStrings(const wchar_t *lhs, const wchar_t *rhs)
{
	return wcscmp(lhs, rhs) < 0;
}

bool StringSortTest()
{
	ff::Vector<ff::String> unsortedStrs;
	ff::Vector<ff::String> strs;
	ff::Vector<const wchar_t *> plainStrs;
	ff::Vector<ff::String *> strPtrs;

	unsortedStrs.Push(ff::String(L"foo"));
	unsortedStrs.Push(ff::String(L"bar"));
	unsortedStrs.Push(ff::String(L"baz"));
	unsortedStrs.Push(ff::String(L"goo"));
	unsortedStrs.Push(ff::String(L"boo"));
	unsortedStrs.Push(ff::String(L"fred"));
	strs = unsortedStrs;

	plainStrs.Push(L"foo");
	plainStrs.Push(L"bar");
	plainStrs.Push(L"baz");
	plainStrs.Push(L"goo");
	plainStrs.Push(L"boo");
	plainStrs.Push(L"fred");

	std::sort(strs.begin(), strs.end());
	std::sort(plainStrs.begin(), plainStrs.end(),
		[](const wchar_t *l, const wchar_t *r)
		{
			return wcscmp(l, r) < 0;
		});

	assertRetVal(strs.Size() == plainStrs.Size(), false);

	for (size_t i = 0; i < strs.Size(); i++)
	{
		assertRetVal(strs[i] == plainStrs[i], false);
	}

	for (size_t i = 0; i < unsortedStrs.Size(); i++)
	{
		strPtrs.Push(&unsortedStrs[i]);
	}

	return true;
}
