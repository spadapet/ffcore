#include "pch.h"
#include "Dict/JsonPersist.h"
#include "Dict/JsonTokenizer.h"
#include "String/StringUtil.h"
#include "Value/Values.h"

bool JsonTokenizerTest()
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
	ff::JsonTokenizer tokenizer(json);

	for (ff::JsonToken token = tokenizer.NextToken();
		token._type != ff::JsonTokenType::None;
		token = tokenizer.NextToken())
	{
		assertRetVal(token._type != ff::JsonTokenType::Error, false);

		ff::ValuePtr value;

		switch (token._type)
		{
		case ff::JsonTokenType::Number:
			value = token.GetValue();
			assertRetVal(value->IsType<ff::DoubleValue>() || value->IsType<ff::IntValue>(), false);
			break;

		case ff::JsonTokenType::String:
			value = token.GetValue();
			assertRetVal(value->IsType<ff::StringValue>(), false);
			break;

		case ff::JsonTokenType::Null:
			value = token.GetValue();
			assertRetVal(value->IsType<ff::NullValue>(), false);
			break;

		case ff::JsonTokenType::True:
			value = token.GetValue();
			assertRetVal(value->IsType<ff::BoolValue>() && value->GetValue<ff::BoolValue>(), false);
			break;

		case ff::JsonTokenType::False:
			value = token.GetValue();
			assertRetVal(value->IsType<ff::BoolValue>() && !value->GetValue<ff::BoolValue>(), false);
			break;
		}
	}

	return true;
}

bool JsonParserTest()
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

	size_t errorPos = 0;
	ff::Dict dict = ff::JsonParse(json, &errorPos);

	assertRetVal(dict.Size() == 5, false);
	ff::Vector<ff::String> names = dict.GetAllNames(true);
	assertRetVal(names[0] == L"foo", false);
	assertRetVal(names[1] == L"identifiers", false);
	assertRetVal(names[2] == L"numbers", false);
	assertRetVal(names[3] == L"obj", false);
	assertRetVal(names[4] == L"string", false);

	assertRetVal(dict.GetValue(ff::String(L"foo"))->IsType<ff::StringValue>(), false);
	assertRetVal(dict.GetValue(ff::String(L"foo"))->GetValue<ff::StringValue>() == L"bar", false);

	return true;
}

bool JsonPrintTest()
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

	ff::String actual = ff::JsonWrite(dict);
	ff::String expect(
		L"{\r\n"
		L"  'foo': 'bar',\r\n"
		L"  'identifiers':\r\n"
		L"  [\r\n"
		L"     true,\r\n"
		L"     false,\r\n"
		L"     null\r\n"
		L"  ],\r\n"
		L"  'numbers':\r\n"
		L"  [\r\n"
		L"     -1,\r\n"
		L"     0,\r\n"
		L"     8.5,\r\n"
		L"     -9.876e+55,\r\n"
		L"     1e-08\r\n"
		L"  ],\r\n"
		L"  'obj':\r\n"
		L"  {\r\n"
		L"    'nested': {},\r\n"
		L"    'nested2': []\r\n"
		L"  },\r\n"
		L"  'string':\r\n"
		L"  [\r\n"
		L"     'Hello',\r\n"
		L"     'a\\'\\r z'\r\n"
		L"  ]\r\n"
		L"}");
	ff::ReplaceAll(expect, '\'', '\"');

	assertRetVal(actual == expect, false);

	return true;
}

bool JsonDeepValue()
{
	ff::String json(
		L"{\n"
		L"  'first': [ 1, 'two', { 'three': [ 10, 20, 30 ] } ],\n"
		L"  'second': { 'nested': { 'nested2': { 'numbers': [ 100, 200, 300 ] } } },\n"
		L"  'arrays': [ [ 1, { 'two': 2 } ], [ 3, 4 ], [ 5, 6, [ 7, 8 ] ] ]\n"
		L"}\n");
	ff::ReplaceAll(json, '\'', '\"');

	ff::Dict dict = ff::JsonParse(json);

	ff::ValuePtr value;
	value = dict.GetValue(ff::String(L"/null"));
	assertRetVal(!value, false);

	value = dict.GetValue(ff::String(L"/first"));
	assertRetVal(value && value->IsType<ff::ValueVectorValue>(), false);

	value = dict.GetValue(ff::String(L"/first/"));
	assertRetVal(!value, false);

	value = dict.GetValue(ff::String(L"/first[1]"));
	assertRetVal(value && value->IsType<ff::StringValue>(), false);

	value = dict.GetValue(ff::String(L"/first[1]/"));
	assertRetVal(!value, false);

	value = dict.GetValue(ff::String(L"/first[2]/null"));
	assertRetVal(!value, false);

	value = dict.GetValue(ff::String(L"/first[2]/three"));
	assertRetVal(value && value->IsType<ff::ValueVectorValue>(), false);

	value = dict.GetValue(ff::String(L"/first[2]/three[1]"));
	assertRetVal(value && value->IsType<ff::IntValue>(), false);

	value = dict.GetValue(ff::String(L"/arrays[0][1]/two"));
	assertRetVal(value && value->IsType<ff::IntValue>(), false);

	value = dict.GetValue(ff::String(L"/arrays[2]"));
	assertRetVal(value && value->IsType<ff::ValueVectorValue>(), false);

	value = dict.GetValue(ff::String(L"/arrays[2][2]"));
	assertRetVal(value && value->IsType<ff::ValueVectorValue>(), false);

	value = dict.GetValue(ff::String(L"/arrays[2][2][1]"));
	assertRetVal(value && value->IsType<ff::IntValue>(), false);

	return true;
}
