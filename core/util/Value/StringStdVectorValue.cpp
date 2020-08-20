#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::StringStdVectorValue::StringStdVectorValue(std::vector<ff::String>&& value)
	: _value(std::move(value))
{
}

const std::vector<ff::String>& ff::StringStdVectorValue::GetValue() const
{
	return _value;
}

ff::Value* ff::StringStdVectorValue::GetStaticValue(std::vector<ff::String>&& value)
{
	return value.empty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::StringStdVectorValue::GetStaticDefaultValue()
{
	static StringStdVectorValue s_empty = std::vector<ff::String>();
	return &s_empty;
}

ff::ValuePtr ff::StringStdVectorValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	return nullptr;
}

ff::ValuePtr ff::StringStdVectorValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->CanHaveIndexedChildren())
	{
		std::vector<ff::String> vec;
		vec.reserve(otherValue->GetIndexChildCount());

		for (size_t i = 0; i < otherValue->GetIndexChildCount(); i++)
		{
			ff::ValuePtr val = otherValue->GetIndexChild(i)->Convert<StringValue>();
			noAssertRetVal(val, nullptr);

			vec.push_back(val->GetValue<StringValue>());
		}

		return ff::Value::New<StringStdVectorValue>(std::move(vec));
	}

	return nullptr;
}

bool ff::StringStdVectorValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::StringStdVectorValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	const std::vector<ff::String>& values = value->GetValue<StringStdVectorValue>();
	return index < values.size() ? ff::Value::New<StringValue>(ff::String(values[index])) : nullptr;
}

size_t ff::StringStdVectorValueType::GetIndexChildCount(const ff::Value* value) const
{
	const std::vector<ff::String>& values = value->GetValue<StringStdVectorValue>();
	return values.size();
}

ff::ValuePtr ff::StringStdVectorValueType::Load(ff::IDataReader* stream) const
{
	DWORD size = 0;
	assertRetVal(ff::LoadData(stream, size), false);

	std::vector<ff::String> vec;
	vec.reserve(size);

	for (size_t i = 0; i < size; i++)
	{
		ff::String str;
		assertRetVal(ff::LoadData(stream, str), false);
		vec.push_back(std::move(str));
	}

	return ff::Value::New<StringStdVectorValue>(std::move(vec));
}

bool ff::StringStdVectorValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const std::vector<ff::String>& vec = value->GetValue<StringStdVectorValue>();
	assertRetVal(ff::SaveData(stream, (DWORD)vec.size()), false);

	for (ff::StringRef str : vec)
	{
		assertRetVal(ff::SaveData(stream, str), false);
	}

	return true;
}
