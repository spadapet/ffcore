#include "pch.h"
#include "String/StringUtil.h"

bool StringTest()
{
	ff::String str1;
	ff::String str2(L"Foobar");

	str1 = str2;
	assertRetVal(str1 == str2, false);

	str1 = str2.c_str();
	assertRetVal(str1 == str2, false);

	str1 = str2[0];
	assertRetVal(str1 == L"F" && str1 == L'F', false);

	str1 = L"";
	str1 += str2;
	assertRetVal(str1 == str2, false);

	str1 = L"";
	str1 += str2.c_str();
	assertRetVal(str1 == str2, false);

	assertRetVal(ff::String(1, L'a') <  ff::String(1, L'b'), false);
	assertRetVal(ff::String(1, L'b') <= ff::String(1, L'b'), false);
	assertRetVal(ff::String(1, L'b') >= ff::String(1, L'a'), false);
	assertRetVal(ff::String(1, L'b') >  ff::String(1, L'a'), false);

	str1.clear();
	str1.append(str2);
	assertRetVal(str1 == str2, false);

	str1.clear();
	str1.append(str2, 3, 3);
	assertRetVal(str1 == str2.substr(3, 3), false);

	str1.assign(str2, 2, 4);
	assertRetVal(str1 == str2.substr(2, 4), false);

	str1 = str2.substr(2, 4);
	assertRetVal(str1 == L"obar", false);

	str1.insert(0, str2.substr(0, 2));
	assertRetVal(str1 == str2, false);

	str1.insert(str1.size(), 5, L'Z');
	assertRetVal(str1 == str2 + L"ZZZZZ", false);

	str1.erase(str1.size() - 5, 5);
	str1.replace(0, 3, L"Bar");
	assertRetVal(str1 == L"Barbar", false);

	str1.replace(3, 3, 3, L'f');
	assertRetVal(str1 == ff::String(L"Barfff"), false);

	str1 = L"Foobar";
	str2 = str1;
	str2.erase(3, 3);
	assertRetVal(str1 == L"Foobar" && str2 == L"Foo", false);

	for(auto iter = str1.begin(); iter != str1.end(); iter++)
	{
		assertRetVal(*iter == "Foobar"[iter - str1.begin()], false);
	}

	str1 = L"Foobar";
	str1.resize(3);
	assertRetVal(str1 == L"Foo", false);
	str1.resize(8);
	str1.replace(3, 5, 5, L' ');
	assertRetVal(str1 == L"Foo     ", false);
	str1.resize(3);
	str1.resize(6, L'o');
	assertRetVal(str1 == L"Fooooo", false);

	str1 = L"Foo";
	str2 = L"Bar";
	str1.swap(str2);
	assertRetVal(str1 == L"Bar" && str2 == L"Foo", false);

	str1 = L"Foobar";
	str2 = L"oo";
	assertRetVal(str1.find(str2) == 1, false);
	assertRetVal(str1.find(str2, 2) == ff::INVALID_SIZE, false);
	assertRetVal(str1.rfind(str2) == 1, false);

	str2 = L"br";
	assertRetVal(str1.find_first_of(str2) == 3, false);
	assertRetVal(str1.find_last_of(str2) == 5, false);
	assertRetVal(str1.find_first_not_of(str2) == 0, false);
	assertRetVal(str1.find_last_not_of(str2) == 4, false);

	str1 = L"Foobarbaz";
	str1.replace(2, 7, str1.c_str(), 6);
	str1.erase(0, 2);
	assertRetVal(str1 == L"Foobar", false);

	str1 = L"Foobar";
	str1.insert(3, str1.c_str(), 6);
	assertRetVal(str1 == L"FooFoobarbar", false);
	str1 += str1;
	str1 += str1;
	str1 += str1;
	assertRetVal(str1.size() == 96, false);

	str1 = ff::String::format_new(L"(%s %d)", L"Cool", 1);
	assertRetVal(str1 == L"(Cool 1)", false);

	str1 = ff::String::format_new(L"%s", L"");
	assertRetVal(str1 == L"", false);

	str1 = L"c:\\foo\\bar";
	assertRetVal(ff::GetPathExtension(str1) == L"", false);
	str1 += L".baz";
	assertRetVal(ff::GetPathExtension(str1) == L"baz", false);
	ff::ChangePathExtension(str1, ff::String(L"goo"));
	assertRetVal(ff::GetPathExtension(str1) == L"goo", false);
	assertRetVal(ff::StripPathTail(str1) == L"c:\\foo", false);

	return true;
}

bool StringHashTest()
{
	const wchar_t *foobar = L"FooBar";
	const wchar_t *foobar2 = L"Foobar";
	assertRetVal(ff::HashFunc(foobar) == ff::HashFunc(ff::String(foobar)), false);
	assertRetVal(ff::HashFunc(foobar) != ff::HashFunc(foobar2), false);
	return true;
}
