#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::StringVectorValue::StringVectorValue(ff::Vector<ff::String>&& value)
	: _value(std::move(value))
{
}

const ff::Vector<ff::String>& ff::StringVectorValue::GetValue() const
{
	return _value;
}

ff::Value* ff::StringVectorValue::GetStaticValue(ff::Vector<ff::String>&& value)
{
	return value.IsEmpty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::StringVectorValue::GetStaticDefaultValue()
{
	static StringVectorValue s_empty = ff::Vector<ff::String>();
	return &s_empty;
}

ff::ValuePtr ff::StringVectorValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	return nullptr;
}

ff::ValuePtr ff::StringVectorValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->CanHaveIndexedChildren())
	{
		ff::Vector<ff::String> vec;
		vec.Reserve(otherValue->GetIndexChildCount());

		for (size_t i = 0; i < otherValue->GetIndexChildCount(); i++)
		{
			ff::ValuePtr val = otherValue->GetIndexChild(i)->Convert<StringValue>();
			noAssertRetVal(val, nullptr);

			vec.Push(val->GetValue<StringValue>());
		}

		return ff::Value::New<StringVectorValue>(std::move(vec));
	}

	return nullptr;
}

bool ff::StringVectorValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::StringVectorValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	const ff::Vector<ff::String>& values = value->GetValue<StringVectorValue>();
	return index < values.Size() ? ff::Value::New<StringValue>(ff::String(values[index])) : nullptr;
}

size_t ff::StringVectorValueType::GetIndexChildCount(const ff::Value* value) const
{
	const ff::Vector<ff::String>& values = value->GetValue<StringVectorValue>();
	return values.Size();
}

ff::ValuePtr ff::StringVectorValueType::Load(ff::IDataReader* stream) const
{
	DWORD size = 0;
	assertRetVal(ff::LoadData(stream, size), false);

	ff::Vector<ff::String> vec;
	vec.Reserve(size);

	for (size_t i = 0; i < size; i++)
	{
		ff::String str;
		assertRetVal(ff::LoadData(stream, str), false);
		vec.Push(std::move(str));
	}

	return ff::Value::New<StringVectorValue>(std::move(vec));
}

bool ff::StringVectorValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::Vector<ff::String>& vec = value->GetValue<StringVectorValue>();
	assertRetVal(ff::SaveData(stream, (DWORD)vec.Size()), false);

	for (ff::StringRef str : vec)
	{
		assertRetVal(ff::SaveData(stream, str), false);
	}

	return true;
}
