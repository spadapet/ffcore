#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::FixedIntVectorValue::FixedIntVectorValue(ff::Vector<ff::FixedInt>&& value)
	: _value(std::move(value))
{
}

const ff::Vector<ff::FixedInt>& ff::FixedIntVectorValue::GetValue() const
{
	return _value;
}

ff::Value* ff::FixedIntVectorValue::GetStaticValue(ff::Vector<ff::FixedInt>&& value)
{
	return value.IsEmpty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::FixedIntVectorValue::GetStaticDefaultValue()
{
	static FixedIntVectorValue s_empty = ff::Vector<ff::FixedInt>();
	return &s_empty;
}

ff::ValuePtr ff::FixedIntVectorValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	return nullptr;
}

ff::ValuePtr ff::FixedIntVectorValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->CanHaveIndexedChildren())
	{
		ff::Vector<ff::FixedInt> vec;
		vec.Reserve(otherValue->GetIndexChildCount());

		for (size_t i = 0; i < otherValue->GetIndexChildCount(); i++)
		{
			ff::ValuePtr val = otherValue->GetIndexChild(i)->Convert<FixedIntValue>();
			noAssertRetVal(val, nullptr);

			vec.Push(val->GetValue<FixedIntValue>());
		}

		return ff::Value::New<FixedIntVectorValue>(std::move(vec));
	}

	return nullptr;
}

bool ff::FixedIntVectorValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::FixedIntVectorValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	const ff::Vector<ff::FixedInt>& values = value->GetValue<FixedIntVectorValue>();
	return index < values.Size() ? ff::Value::New<FixedIntValue>(values[index]) : nullptr;
}

size_t ff::FixedIntVectorValueType::GetIndexChildCount(const ff::Value* value) const
{
	const ff::Vector<ff::FixedInt>& values = value->GetValue<FixedIntVectorValue>();
	return values.Size();
}

ff::ValuePtr ff::FixedIntVectorValueType::Load(ff::IDataReader* stream) const
{
	DWORD size = 0;
	assertRetVal(ff::LoadData(stream, size), false);

	ff::Vector<ff::FixedInt> vec;
	if (size)
	{
		vec.Resize(size);
		assertRetVal(ff::LoadBytes(stream, vec.Data(), vec.ByteSize()), false);
	}

	return ff::Value::New<FixedIntVectorValue>(std::move(vec));
}

bool ff::FixedIntVectorValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::Vector<ff::FixedInt>& src = value->GetValue<FixedIntVectorValue>();
	assertRetVal(ff::SaveData(stream, (DWORD)src.Size()), false);
	assertRetVal(src.IsEmpty() || ff::SaveBytes(stream, src.ConstData(), src.ByteSize()), false);
	return true;
}
