#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"
#include "String/StringUtil.h"

ff::GuidValue::GuidValue(REFGUID value)
	: _value(value)
{
}

REFGUID ff::GuidValue::GetValue() const
{
	return _value;
}

ff::Value* ff::GuidValue::GetStaticValue(REFGUID value)
{
	return value == GUID_NULL ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::GuidValue::GetStaticDefaultValue()
{
	static GuidValue s_null(GUID_NULL);
	return &s_null;
}

ff::ValuePtr ff::GuidValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	REFGUID src = value->GetValue<GuidValue>();

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::StringFromGuid(src));
	}

	return nullptr;
}

ff::ValuePtr ff::GuidValueType::Load(ff::IDataReader* stream) const
{
	GUID data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<GuidValue>(data);
}

bool ff::GuidValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	REFGUID data = value->GetValue<GuidValue>();
	return ff::SaveData(stream, data);
}
