#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::FloatVectorValue::FloatVectorValue(ff::Vector<float>&& value)
	: _value(std::move(value))
{
}

const ff::Vector<float>& ff::FloatVectorValue::GetValue() const
{
	return _value;
}

ff::Value* ff::FloatVectorValue::GetStaticValue(ff::Vector<float>&& value)
{
	return value.IsEmpty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::FloatVectorValue::GetStaticDefaultValue()
{
	static FloatVectorValue s_empty = ff::Vector<float>();
	return &s_empty;
}

ff::ValuePtr ff::FloatVectorValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	return nullptr;
}

ff::ValuePtr ff::FloatVectorValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->CanHaveIndexedChildren())
	{
		ff::Vector<float> vec;
		vec.Reserve(otherValue->GetIndexChildCount());

		for (size_t i = 0; i < otherValue->GetIndexChildCount(); i++)
		{
			ff::ValuePtr val = otherValue->GetIndexChild(i)->Convert<FloatValue>();
			noAssertRetVal(val, nullptr);

			vec.Push(val->GetValue<FloatValue>());
		}

		return ff::Value::New<FloatVectorValue>(std::move(vec));
	}

	return nullptr;
}

bool ff::FloatVectorValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::FloatVectorValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	const ff::Vector<float>& values = value->GetValue<FloatVectorValue>();
	return index < values.Size() ? ff::Value::New<FloatValue>(values[index]) : nullptr;
}

size_t ff::FloatVectorValueType::GetIndexChildCount(const ff::Value* value) const
{
	const ff::Vector<float>& values = value->GetValue<FloatVectorValue>();
	return values.Size();
}

ff::ValuePtr ff::FloatVectorValueType::Load(ff::IDataReader* stream) const
{
	DWORD size = 0;
	assertRetVal(ff::LoadData(stream, size), false);

	ff::Vector<float> vec;
	if (size)
	{
		vec.Resize(size);
		assertRetVal(ff::LoadBytes(stream, vec.Data(), vec.ByteSize()), false);
	}

	return ff::Value::New<FloatVectorValue>(std::move(vec));
}

bool ff::FloatVectorValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::Vector<float>& src = value->GetValue<FloatVectorValue>();
	assertRetVal(ff::SaveData(stream, (DWORD)src.Size()), false);
	assertRetVal(src.IsEmpty() || ff::SaveBytes(stream, src.ConstData(), src.ByteSize()), false);
	return true;
}
