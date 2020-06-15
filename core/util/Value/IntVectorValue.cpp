#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::IntVectorValue::IntVectorValue(ff::Vector<int>&& value)
	: _value(std::move(value))
{
}

const ff::Vector<int>& ff::IntVectorValue::GetValue() const
{
	return _value;
}

ff::Value* ff::IntVectorValue::GetStaticValue(ff::Vector<int>&& value)
{
	return value.IsEmpty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::IntVectorValue::GetStaticDefaultValue()
{
	static IntVectorValue s_empty = ff::Vector<int>();
	return &s_empty;
}

ff::ValuePtr ff::IntVectorValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	return nullptr;
}

ff::ValuePtr ff::IntVectorValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->CanHaveIndexedChildren())
	{
		ff::Vector<int> vec;
		vec.Reserve(otherValue->GetIndexChildCount());

		for (size_t i = 0; i < otherValue->GetIndexChildCount(); i++)
		{
			ff::ValuePtr val = otherValue->GetIndexChild(i)->Convert<IntValue>();
			noAssertRetVal(val, nullptr);

			vec.Push(val->GetValue<IntValue>());
		}

		return ff::Value::New<IntVectorValue>(std::move(vec));
	}

	return nullptr;
}

bool ff::IntVectorValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::IntVectorValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	const ff::Vector<int>& values = value->GetValue<IntVectorValue>();
	return index < values.Size() ? ff::Value::New<IntValue>(values[index]) : nullptr;
}

size_t ff::IntVectorValueType::GetIndexChildCount(const ff::Value* value) const
{
	const ff::Vector<int>& values = value->GetValue<IntVectorValue>();
	return values.Size();
}

ff::ValuePtr ff::IntVectorValueType::Load(ff::IDataReader* stream) const
{
	DWORD size = 0;
	assertRetVal(ff::LoadData(stream, size), false);

	ff::Vector<int> vec;
	if (size)
	{
		vec.Resize(size);
		assertRetVal(ff::LoadBytes(stream, vec.Data(), vec.ByteSize()), false);
	}

	return ff::Value::New<IntVectorValue>(std::move(vec));
}

bool ff::IntVectorValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::Vector<int>& src = value->GetValue<IntVectorValue>();
	assertRetVal(ff::SaveData(stream, (DWORD)src.Size()), false);
	assertRetVal(src.IsEmpty() || ff::SaveBytes(stream, src.ConstData(), src.ByteSize()), false);
	return true;
}
