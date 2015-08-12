#include "pch.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/SavedData.h"
#include "Dict/DictPersist.h"
#include "Value/Values.h"

ff::DictValue::DictValue(ff::Dict&& value)
	: _value(std::move(value))
{
}

const ff::Dict& ff::DictValue::GetValue() const
{
	return _value;
}

ff::Value* ff::DictValue::GetStaticValue(ff::Dict&& value)
{
	return value.IsEmpty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::DictValue::GetStaticDefaultValue()
{
	static DictValue s_empty = ff::Dict();
	return &s_empty;
}

ff::ValuePtr ff::DictValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::Dict& src = value->GetValue<DictValue>();

	if (type == typeid(DataValue))
	{
		ff::ComPtr<ff::IData> data = ff::SaveDict(src);
		if (data)
		{
			return ff::Value::New<DataValue>(data);
		}
	}

	if (type == typeid(SavedDataValue))
	{
		ff::ComPtr<ff::IData> data = ff::SaveDict(src);
		ff::ComPtr<ff::ISavedData> savedData;
		if (data && ff::CreateLoadedDataFromMemory(data, false, &savedData))
		{
			return ff::Value::New<SavedDataValue>(savedData);
		}
	}

	if (type == typeid(SavedDictValue))
	{
		ff::ComPtr<ff::IData> data = ff::SaveDict(src);
		ff::ComPtr<ff::ISavedData> savedData;
		if (data && ff::CreateLoadedDataFromMemory(data, false, &savedData))
		{
			return ff::Value::New<SavedDictValue>(savedData);
		}
	}

	return nullptr;
}

ff::ValuePtr ff::DictValueType::GetNamedChild(const ff::Value* value, ff::StringRef name) const
{
	return value->GetValue<DictValue>().GetValue(name);
}

bool ff::DictValueType::CanHaveNamedChildren() const
{
	return true;
}

ff::Vector<ff::String> ff::DictValueType::GetChildNames(const ff::Value* value) const
{
	return value->GetValue<DictValue>().GetAllNames();
}

ff::ValuePtr ff::DictValueType::Load(ff::IDataReader* stream) const
{
	return ff::SavedDataValueType::Load(stream, true);
}

bool ff::DictValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	return value->Convert<SavedDataValue>()->Save(stream);
}
