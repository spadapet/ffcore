#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::FixedIntValue::FixedIntValue(ff::FixedInt value)
	: _value(value)
{
}

ff::FixedInt ff::FixedIntValue::GetValue() const
{
	return _value;
}

ff::Value* ff::FixedIntValue::GetStaticValue(ff::FixedInt value)
{
	return value == 0_f ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::FixedIntValue::GetStaticDefaultValue()
{
	static FixedIntValue s_zero(0_f);
	return &s_zero;
}

ff::ValuePtr ff::FixedIntValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	ff::FixedInt src = value->GetValue<FixedIntValue>();

	if (type == typeid(BoolValue))
	{
		return ff::Value::New<BoolValue>(src);
	}

	if (type == typeid(DoubleValue))
	{
		return ff::Value::New<DoubleValue>(src);
	}

	if (type == typeid(FloatValue))
	{
		return ff::Value::New<FloatValue>(src);
	}

	if (type == typeid(IntValue))
	{
		return ff::Value::New<IntValue>(src);
	}

	if (type == typeid(SizeValue) && (double)src >= 0)
	{
		return ff::Value::New<SizeValue>((size_t)(double)src);
	}

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"%g", (double)src));
	}

	return nullptr;
}

ff::ValuePtr ff::FixedIntValueType::Load(ff::IDataReader* stream) const
{
	ff::FixedInt data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<FixedIntValue>(data);
}

bool ff::FixedIntValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	ff::FixedInt data = value->GetValue<FixedIntValue>();
	return ff::SaveData(stream, data);
}
