#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

static ff::StaticString s_falseName(L"false");
static ff::StaticString s_trueName(L"true");

ff::BoolValue::BoolValue(bool value)
	: _value(value)
{
}

bool ff::BoolValue::GetValue() const
{
	return _value;
}

ff::Value* ff::BoolValue::GetStaticValue(bool value)
{
	static BoolValue s_true(true);
	return value ? &s_true : GetStaticDefaultValue();
}

ff::Value* ff::BoolValue::GetStaticDefaultValue()
{
	static BoolValue s_false(false);
	return &s_false;
}

ff::ValuePtr ff::BoolValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	bool src = value->GetValue<BoolValue>();

	if (type == typeid(DoubleValue))
	{
		return ff::Value::New<DoubleValue>(src ? 1.0 : 0.0);
	}

	if (type == typeid(FixedIntValue))
	{
		return ff::Value::New<FixedIntValue>(src);
	}

	if (type == typeid(FloatValue))
	{
		return ff::Value::New<FloatValue>(src ? 1.0f : 0.0f);
	}

	if (type == typeid(IntValue))
	{
		return ff::Value::New<IntValue>(src ? 1 : 0);
	}

	if (type == typeid(SizeValue))
	{
		return ff::Value::New<SizeValue>(src ? 1 : 0);
	}

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String(src ? s_trueName : s_falseName));
	}

	return nullptr;
}

ff::ValuePtr ff::BoolValueType::ConvertFrom(const ff::Value* otherValue) const
{
	ff::ComPtr<IUnknown> obj = otherValue->GetComObject();
	if (obj)
	{
		return ff::Value::New<BoolValue>(obj != nullptr);
	}

	return nullptr;
}

ff::ValuePtr ff::BoolValueType::Load(ff::IDataReader* stream) const
{
	bool data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<BoolValue>(data);
}

bool ff::BoolValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	bool data = value->GetValue<BoolValue>();
	return ff::SaveData(stream, data);
}
