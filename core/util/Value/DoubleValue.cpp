#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::DoubleValue::DoubleValue(double value)
	: _value(value)
{
}

double ff::DoubleValue::GetValue() const
{
	return _value;
}

ff::Value* ff::DoubleValue::GetStaticValue(double value)
{
	return value == 0 ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::DoubleValue::GetStaticDefaultValue()
{
	static DoubleValue s_zero(0);
	return &s_zero;
}

ff::ValuePtr ff::DoubleValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	double src = value->GetValue<DoubleValue>();

	if (type == typeid(BoolValue))
	{
		return ff::Value::New<BoolValue>(src != 0.0);
	}

	if (type == typeid(FixedIntValue))
	{
		return ff::Value::New<FixedIntValue>(src);
	}

	if (type == typeid(FloatValue))
	{
		return ff::Value::New<FloatValue>((float)src);
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

ff::ValuePtr ff::DoubleValueType::Load(ff::IDataReader* stream) const
{
	double data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<DoubleValue>(data);
}

bool ff::DoubleValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	double data = value->GetValue<DoubleValue>();
	return ff::SaveData(stream, data);
}
