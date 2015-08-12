#include "pch.h"
#include "Data/DataPersist.h"
#include "String/StringUtil.h"
#include "Value/Values.h"

ff::hash_t ParseHash(ff::StringRef key);

ff::StringValue::StringValue(ff::String&& value)
	: _value(std::move(value))
{
}

ff::StringValue::StringValue(ff::StringRef value)
	: _value(value)
{
}

ff::StringRef ff::StringValue::GetValue() const
{
	return _value;
}

ff::Value* ff::StringValue::GetStaticValue(ff::String&& value)
{
	return value.empty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::StringValue::GetStaticValue(ff::StringRef value)
{
	return value.empty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::StringValue::GetStaticDefaultValue()
{
	static StringValue s_empty = ff::String();
	return &s_empty;
}

ff::ValuePtr ff::StringValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	ff::StringRef src = value->GetValue<StringValue>();
	const wchar_t* start = src.c_str();
	wchar_t* end = nullptr;
	int charsRead = 0;

	if (type == typeid(BoolValue))
	{
		if (src.empty() || src == L"false" || src == L"no" || src == L"0")
		{
			return ff::Value::New<BoolValue>(false);
		}
		else if (src == L"true" || src == L"yes" || src == L"1")
		{
			return ff::Value::New<BoolValue>(true);
		}
	}

	if (type == typeid(DoubleValue))
	{
		double val = std::wcstod(start, &end);
		if (end > start && !*end)
		{
			return ff::Value::New<DoubleValue>(val);
		}
	}

	if (type == typeid(FixedIntValue))
	{
		double val = std::wcstod(start, &end);
		if (end > start && !*end)
		{
			return ff::Value::New<FixedIntValue>(val);
		}
	}

	if (type == typeid(FloatValue))
	{
		double val = std::wcstod(start, &end);
		if (end > start && !*end)
		{
			return ff::Value::New<FloatValue>((float)val);
		}
	}

	if (type == typeid(IntValue))
	{
		long val = std::wcstol(start, &end, 10);
		if (end > start && !*end)
		{
			return ff::Value::New<IntValue>(val);
		}
	}

	if (type == typeid(SizeValue))
	{
		unsigned long val = std::wcstoul(start, &end, 10);
		if (end > start && !*end)
		{
			return ff::Value::New<SizeValue>(val);
		}
	}

	if (type == typeid(PointIntValue))
	{
		ff::PointInt point;
		if (::_snwscanf_s(start, src.size(), L"[ %d, %d ]%n", &point.x, &point.y, &charsRead) == 2 &&
			(size_t)charsRead == src.size())
		{
			return ff::Value::New<PointIntValue>(point);
		}
	}

	if (type == typeid(PointFloatValue))
	{
		ff::PointFloat point;
		if (::_snwscanf_s(start, src.size(), L"[ %g, %g ]%n", &point.x, &point.y, &charsRead) == 2 &&
			(size_t)charsRead == src.size())
		{
			return ff::Value::New<PointFloatValue>(point);
		}
	}

	if (type == typeid(PointDoubleValue))
	{
		ff::PointDouble point;
		if (::_snwscanf_s(start, src.size(), L"[ %lg, %lg ]%n", &point.x, &point.y, &charsRead) == 2 &&
			(size_t)charsRead == src.size())
		{
			return ff::Value::New<PointDoubleValue>(point);
		}
	}

	if (type == typeid(PointFixedIntValue))
	{
		ff::PointDouble point;
		if (::_snwscanf_s(start, src.size(), L"[ %lg, %lg ]%n", &point.x, &point.y, &charsRead) == 2 &&
			(size_t)charsRead == src.size())
		{
			return ff::Value::New<PointFixedIntValue>(point.ToType<ff::FixedInt>());
		}
	}

	if (type == typeid(RectIntValue))
	{
		ff::RectInt rect;
		if (::_snwscanf_s(start, src.size(), L"[ %d, %d, %d, %d ]%n", &rect.left, &rect.top, &rect.right, &rect.bottom, &charsRead) == 4 &&
			(size_t)charsRead == src.size())
		{
			return ff::Value::New<RectIntValue>(rect);
		}
	}

	if (type == typeid(RectFloatValue))
	{
		ff::RectFloat rect;
		if (::_snwscanf_s(start, src.size(), L"[ %g, %g, %g, %g ]%n", &rect.left, &rect.top, &rect.right, &rect.bottom, &charsRead) == 4 &&
			(size_t)charsRead == src.size())
		{
			return ff::Value::New<RectFloatValue>(rect);
		}
	}

	if (type == typeid(RectDoubleValue))
	{
		ff::RectDouble rect;
		if (::_snwscanf_s(start, src.size(), L"[ %lg, %lg, %lg, %lg ]%n", &rect.left, &rect.top, &rect.right, &rect.bottom, &charsRead) == 4 &&
			(size_t)charsRead == src.size())
		{
			return ff::Value::New<RectDoubleValue>(rect);
		}
	}

	if (type == typeid(RectFixedIntValue))
	{
		ff::RectDouble rect;
		if (::_snwscanf_s(start, src.size(), L"[ %lg, %lg, %lg, %lg ]%n", &rect.left, &rect.top, &rect.right, &rect.bottom, &charsRead) == 4 &&
			(size_t)charsRead == src.size())
		{
			return ff::Value::New<RectFixedIntValue>(rect.ToType<ff::FixedInt>());
		}
	}

	if (type == typeid(GuidValue))
	{
		GUID guid;
		if (ff::StringToGuid(src, guid))
		{
			return ff::Value::New<GuidValue>(guid);
		}
	}

	if (type == typeid(HashValue))
	{
		ff::hash_t hash = ParseHash(src);
		if (hash)
		{
			return ff::Value::New<HashValue>(hash);
		}
	}

	return nullptr;
}

ff::ValuePtr ff::StringValueType::Load(ff::IDataReader* stream) const
{
	ff::String data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<StringValue>(std::move(data));
}

bool ff::StringValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	ff::StringRef data = value->GetValue<StringValue>();
	return ff::SaveData(stream, data);
}
