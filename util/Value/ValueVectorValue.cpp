#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/ValuePersist.h"
#include "Value/Values.h"

ff::ValueVectorValue::ValueVectorValue(ff::Vector<ff::ValuePtr>&& value)
	: _value(std::move(value))
{
}

const ff::Vector<ff::ValuePtr>& ff::ValueVectorValue::GetValue() const
{
	return _value;
}

ff::Value* ff::ValueVectorValue::GetStaticValue(ff::Vector<ff::ValuePtr>&& value)
{
	return value.IsEmpty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::ValueVectorValue::GetStaticDefaultValue()
{
	static ValueVectorValue s_empty = ff::Vector<ff::ValuePtr>();
	return &s_empty;
}

ff::ValuePtr ff::ValueVectorValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	return nullptr;
}

ff::ValuePtr ff::ValueVectorValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->CanHaveIndexedChildren())
	{
		ff::Vector<ff::ValuePtr> vec;
		vec.Reserve(otherValue->GetIndexChildCount());

		for (size_t i = 0; i < otherValue->GetIndexChildCount(); i++)
		{
			vec.Push(otherValue->GetIndexChild(i));
		}

		return ff::Value::New<ValueVectorValue>(std::move(vec));
	}

	return nullptr;
}

bool ff::ValueVectorValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::ValueVectorValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	const ff::Vector<ff::ValuePtr>& values = value->GetValue<ValueVectorValue>();
	return index < values.Size() ? values[index] : nullptr;
}

size_t ff::ValueVectorValueType::GetIndexChildCount(const ff::Value* value) const
{
	const ff::Vector<ff::ValuePtr>& values = value->GetValue<ValueVectorValue>();
	return values.Size();
}

ff::ValuePtr ff::ValueVectorValueType::Load(ff::IDataReader* stream) const
{
	DWORD size = 0;
	assertRetVal(ff::LoadData(stream, size), false);

	ff::Vector<ff::ValuePtr> vec;
	vec.Reserve(size);

	for (size_t i = 0; i < size; i++)
	{
		ff::ValuePtr val = ff::LoadTypedValue(stream);
		assertRetVal(val, false);
		vec.Push(std::move(val));
	}

	return ff::Value::New<ValueVectorValue>(std::move(vec));
}

bool ff::ValueVectorValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::Vector<ff::ValuePtr>& vec = value->GetValue<ValueVectorValue>();
	assertRetVal(ff::SaveData(stream, (DWORD)vec.Size()), false);

	for (const ff::ValuePtr& val : vec)
	{
		assertRetVal(ff::SaveTypedValue(stream, val), false);
	}

	return true;
}
