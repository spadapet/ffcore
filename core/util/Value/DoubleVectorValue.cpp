#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::DoubleVectorValue::DoubleVectorValue(ff::Vector<double>&& value)
	: _value(std::move(value))
{
}

const ff::Vector<double>& ff::DoubleVectorValue::GetValue() const
{
	return _value;
}

ff::Value* ff::DoubleVectorValue::GetStaticValue(ff::Vector<double>&& value)
{
	return value.IsEmpty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::DoubleVectorValue::GetStaticDefaultValue()
{
	static DoubleVectorValue s_empty = ff::Vector<double>();
	return &s_empty;
}

ff::ValuePtr ff::DoubleVectorValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	return nullptr;
}

ff::ValuePtr ff::DoubleVectorValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->CanHaveIndexedChildren())
	{
		ff::Vector<double> vec;
		vec.Reserve(otherValue->GetIndexChildCount());

		for (size_t i = 0; i < otherValue->GetIndexChildCount(); i++)
		{
			ff::ValuePtr val = otherValue->GetIndexChild(i)->Convert<DoubleValue>();
			noAssertRetVal(val, nullptr);

			vec.Push(val->GetValue<DoubleValue>());
		}

		return ff::Value::New<DoubleVectorValue>(std::move(vec));
	}

	return nullptr;
}

bool ff::DoubleVectorValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::DoubleVectorValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	const ff::Vector<double>& values = value->GetValue<DoubleVectorValue>();
	return index < values.Size() ? ff::Value::New<DoubleValue>(values[index]) : nullptr;
}

size_t ff::DoubleVectorValueType::GetIndexChildCount(const ff::Value* value) const
{
	const ff::Vector<double>& values = value->GetValue<DoubleVectorValue>();
	return values.Size();
}

ff::ValuePtr ff::DoubleVectorValueType::Load(ff::IDataReader* stream) const
{
	DWORD size = 0;
	assertRetVal(ff::LoadData(stream, size), false);

	ff::Vector<double> vec;
	vec.Resize(size);
	assertRetVal(ff::LoadBytes(stream, vec.Data(), vec.ByteSize()), false);

	return ff::Value::New<DoubleVectorValue>(std::move(vec));
}

bool ff::DoubleVectorValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::Vector<double>& src = value->GetValue<DoubleVectorValue>();
	assertRetVal(ff::SaveData(stream, (DWORD)src.Size()), false);
	assertRetVal(ff::SaveBytes(stream, src.ConstData(), src.ByteSize()), false);
	return true;
}
