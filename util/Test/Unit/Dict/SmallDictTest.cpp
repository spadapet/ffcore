#include "pch.h"
#include "Data/Data.h"
#include "Data/DataWriterReader.h"
#include "Dict/DictPersist.h"
#include "Dict/JsonPersist.h"
#include "String/StringUtil.h"
#include "Value/Values.h"

bool SmallDictTest()
{
	ff::String foo(L"Foo");
	ff::String bar(L"Bar");

	ff::SmallDict dict;
	assertRetVal(dict.Size() == 0, false);

	ff::ValuePtr v1 = ff::Value::New<ff::IntValue>(20);
	ff::ValuePtr v2 = ff::Value::New<ff::StringValue>(ff::String(L"Value"));

	dict.Add(foo, v1);
	assertRetVal(dict.GetValue(foo) == v1, false);
	assertRetVal(dict.Size() == 1, false);

	dict.Set(foo, v2);
	assertRetVal(dict.GetValue(foo) == v2, false);
	assertRetVal(dict.Size() == 1, false);

	dict.Add(foo, v1);
	assertRetVal(dict.GetValue(foo) == v2, false);
	assertRetVal(dict.Size() == 2, false);

	dict.Set(bar, v1);
	assertRetVal(dict.GetValue(bar) == v1, false);
	assertRetVal(dict.Size() == 3, false);

	assertRetVal(dict.KeyAt(0) == foo, false);
	assertRetVal(dict.KeyAt(1) == foo, false);
	assertRetVal(dict.KeyAt(2) == bar, false);

	dict.Remove(foo);
	assertRetVal(dict.GetValue(foo) == nullptr, false);
	assertRetVal(dict.Size() == 1, false);
	assertRetVal(dict.KeyAt((size_t)0) == bar, false);

	dict.Add(ff::String(L"2"), v1);
	dict.Add(ff::String(L"3"), v1);
	dict.Add(ff::String(L"4"), v1);
	dict.Add(ff::String(L"5"), v1);
	dict.Add(ff::String(L"6"), v1);
	dict.Add(ff::String(L"7"), v1);
	dict.Add(ff::String(L"8"), v1);
	assertRetVal(dict.Size() == 8, false);

	for (size_t i = 2; i < 8; i++)
	{
		assertRetVal(dict.ValueAt(i) == v1, false);
	}

	ff::SmallDict dict2 = dict;
	assertRetVal(dict2.Size() == 8, false);

	ff::SmallDict dict3(std::move(dict2));
	assertRetVal(dict3.Size() == 8, false);
	assertRetVal(dict2.Size() == 0, false);

	dict.Clear();
	assertRetVal(dict.GetValue(foo) == nullptr, false);
	assertRetVal(dict.Size() == 0, false);

	return true;
}

bool SmallDictPersistTest()
{
	ff::String json(
		L"{\n"
		L"  // Test comment\n"
		L"  /* Another test comment */\n"
		L"  'foo': 'bar',\n"
		L"  'obj' : { 'nested': {}, 'nested2': [] },\n"
		L"  'numbers' : [ -1, 0, 8.5, -98.76e54, 1E-8 ],\n"
		L"  'identifiers' : [ true, false, null ],\n"
		L"  'string' : [ 'Hello', 'a\\'\\r\\u0020z' ],\n"
		L"}\n");
	ff::ReplaceAll(json, '\'', '\"');

	ff::Dict dict = ff::JsonParse(json);

	ff::ComPtr<ff::IData> data = ff::SaveDict(dict);
	assertRetVal(data, false);

	ff::ComPtr<ff::IDataReader> reader;
	assertRetVal(ff::CreateDataReader(data, 0, &reader), false);

	ff::Dict loadedDict;
	assertRetVal(ff::LoadDict(reader, loadedDict), false);
	assertRetVal(loadedDict.Size() == dict.Size(), false);

	ff::String expected = ff::JsonWrite(dict);
	ff::String actual = ff::JsonWrite(loadedDict);
	assertRetVal(actual == expected, false);

	return true;
}
