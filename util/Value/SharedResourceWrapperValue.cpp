#include "pch.h"
#include "Data/DataPersist.h"
#include "Resource/ResourceValue.h"
#include "Value/Values.h"

ff::SharedResourceWrapperValue::SharedResourceWrapperValue(const ff::SharedResourceValue& value)
	: _value(value)
{
}

const ff::SharedResourceValue& ff::SharedResourceWrapperValue::GetValue() const
{
	return _value;
}

ff::Value* ff::SharedResourceWrapperValue::GetStaticValue(const ff::SharedResourceValue& value)
{
	return !value ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::SharedResourceWrapperValue::GetStaticDefaultValue()
{
	static SharedResourceWrapperValue s_null(nullptr);
	return &s_null;
}

ff::ValuePtr ff::SharedResourceWrapperValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	ff::SharedResourceValue src = value->GetValue<SharedResourceWrapperValue>();

	if (src && !src->IsValid())
	{
		src = src->GetNewValue();
	}

	if (src)
	{
		return src->GetValue()->Convert(type);
	}

	return nullptr;
}

ff::ValuePtr ff::SharedResourceWrapperValueType::Load(ff::IDataReader* stream) const
{
	// Load as a string, the resource loader will convert strings to resource references
	ff::String str;
	assertRetVal(ff::LoadData(stream, str), false);
	str.insert(0, ff::REF_PREFIX);
	return ff::Value::New<StringValue>(std::move(str));
}

bool ff::SharedResourceWrapperValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	ff::SharedResourceValue res = value->GetValue<SharedResourceWrapperValue>();
	ff::StringRef name = (res != nullptr) ? res->GetName() : ff::GetEmptyString();
	assertRetVal(ff::SaveData(stream, name), false);
	return true;
}

ff::ComPtr<IUnknown> ff::SharedResourceWrapperValueType::GetComObject(const ff::Value* value) const
{
	ff::ValuePtrT<ff::ObjectValue> objectValue = value;
	return objectValue.GetValue();
}

ff::String ff::SharedResourceWrapperValueType::Print(const ff::Value* value)
{
	const ff::SharedResourceValue& resValue = value->GetValue<SharedResourceWrapperValue>();
	return ff::String::format_new(L"<Resource: %s>", resValue ? resValue->GetName().c_str() : L"<null>");
}
