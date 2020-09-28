#include "pch.h"
#include "COM/ComObject.h"
#include "Data/DataPersist.h"
#include "Value/ValuePersist.h"
#include "Value/Values.h"

ff::ObjectValue::ObjectValue(IUnknown* value)
	: _value(value)
{
}

const ff::ComPtr<IUnknown>& ff::ObjectValue::GetValue() const
{
	return _value;
}

ff::Value* ff::ObjectValue::GetStaticValue(IUnknown* value)
{
	return !value ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::ObjectValue::GetStaticDefaultValue()
{
	static ObjectValue s_null(nullptr);
	return &s_null;
}

ff::ValuePtr ff::ObjectValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::ComPtr<IUnknown>& src = value->GetValue<ObjectValue>();

	if (type == typeid(DictValue))
	{
		ff::ComPtr<ff::IResourcePersist> resObj;
		if (resObj.QueryFrom(src))
		{
			ff::Dict dict;
			if (resObj->SaveToCache(dict))
			{
				return ff::Value::New<DictValue>(std::move(dict));
			}
		}
	}

	return nullptr;
}

ff::ValuePtr ff::ObjectValueType::Load(ff::IDataReader* stream) const
{
	// Just load the dict, don't convert it into an object
	return ff::LoadTypedValue(stream);
}

bool ff::ObjectValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	ff::Dict dict;
	assertRetVal(ff::SaveResourceToCache(value->GetValue<ObjectValue>(), dict), false);

	ff::ValuePtr dictValue = ff::Value::New<DictValue>(std::move(dict));
	assertRetVal(ff::SaveTypedValue(stream, dictValue), false);
	return true;
}

ff::ComPtr<IUnknown> ff::ObjectValueType::GetComObject(const ff::Value* value) const
{
	return value->GetValue<ObjectValue>().Interface();
}

ff::String ff::ObjectValueType::Print(const ff::Value* value)
{
	ff::ComPtr<IUnknown> obj = value->GetValue<ObjectValue>();
	ff::ComPtr<ff::IComObject> comObj;
	if (comObj.QueryFrom(obj))
	{
		return ff::String::format_new(L"<Object: %s>", comObj->GetComClassName().c_str());
	}

	return ff::String::format_new(L"<Object>");
}
