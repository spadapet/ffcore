#include "pch.h"
#include "Value/NullValue.h"

static ff::StaticString s_nullName(L"null");

ff::NullValue::NullValue()
{
}

std::nullptr_t ff::NullValue::GetValue() const
{
	return nullptr;
}

ff::Value* ff::NullValue::GetStaticValue()
{
	return GetStaticDefaultValue();
}

ff::Value* ff::NullValue::GetStaticDefaultValue()
{
	static NullValue s_value;
	return &s_value;
}

ff::ValuePtr ff::NullValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String(s_nullName));
	}

	return nullptr;
}

ff::ValuePtr ff::NullValueType::Load(ff::IDataReader* stream) const
{
	return ff::Value::New<NullValue>();
}

bool ff::NullValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	return true;
}
