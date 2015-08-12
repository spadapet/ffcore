#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::FloatValue::FloatValue(float value)
	: _value(value)
{
}

float ff::FloatValue::GetValue() const
{
	return _value;
}

ff::Value* ff::FloatValue::GetStaticValue(float value)
{
	return value == 0 ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::FloatValue::GetStaticDefaultValue()
{
	static FloatValue s_zero(0);
	return &s_zero;
}

ff::ValuePtr ff::FloatValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	float src = value->GetValue<FloatValue>();

	if (type == typeid(BoolValue))
	{
		return ff::Value::New<BoolValue>(src != 0.0f);
	}

	if (type == typeid(DoubleValue))
	{
		return ff::Value::New<DoubleValue>(src);
	}

	if (type == typeid(FixedIntValue))
	{
		return ff::Value::New<FixedIntValue>(src);
	}

	if (type == typeid(IntValue))
	{
		return ff::Value::New<IntValue>((int)src);
	}

	if (type == typeid(SizeValue) && src >= 0)
	{
		return ff::Value::New<SizeValue>((size_t)src);
	}

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"%g", src));
	}

	return nullptr;
}

ff::ValuePtr ff::FloatValueType::Load(ff::IDataReader* stream) const
{
	float data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<FloatValue>(data);
}

bool ff::FloatValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	float data = value->GetValue<FloatValue>();
	return ff::SaveData(stream, data);
}
