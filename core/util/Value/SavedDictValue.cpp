#include "pch.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/DictPersist.h"
#include "Value/Values.h"

ff::SavedDictValue::SavedDictValue(ff::ISavedData* value)
	: _value(value)
{
}

ff::SavedDictValue::SavedDictValue(const ff::Dict& value, bool compress)
{
	ff::ComPtr<ff::IData> data = ff::SaveDict(value);
	verify(ff::CreateLoadedDataFromMemory(data, compress, &_value));
}

const ff::ComPtr<ff::ISavedData>& ff::SavedDictValue::GetValue() const
{
	return _value;
}

ff::Value* ff::SavedDictValue::GetStaticValue(ff::ISavedData* value)
{
	return !value ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::SavedDictValue::GetStaticValue(const ff::Dict& value, bool compress)
{
	return nullptr;
}

ff::Value* ff::SavedDictValue::GetStaticDefaultValue()
{
	static SavedDictValue s_null(nullptr);
	return &s_null;
}

ff::ValuePtr ff::SavedDictValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::ComPtr<ff::ISavedData>& src = value->GetValue<SavedDictValue>();

	if (type == typeid(DataValue) && src)
	{
		ff::ComPtr<ff::ISavedData> cloneSavedData;
		if (src->Clone(&cloneSavedData))
		{
			ff::ComPtr<ff::IData> data = cloneSavedData->Load();
			if (data)
			{
				return ff::Value::New<DataValue>(data);
			}
		}
	}

	if (type == typeid(SavedDataValue) && src)
	{
		return ff::Value::New<SavedDataValue>(src);
	}

	if (type == typeid(DictValue) && src)
	{
		ff::ComPtr<ff::ISavedData> savedData;
		if (src->Clone(&savedData))
		{
			ff::ComPtr<ff::IData> data = savedData->Load();
			ff::ComPtr<ff::IDataReader> dataReader;

			if (data && ff::CreateDataReader(data, 0, &dataReader))
			{
				ff::Dict dict;
				if (ff::LoadDict(dataReader, dict))
				{
					return ff::Value::New<DictValue>(std::move(dict));
				}
			}
		}
	}

	return nullptr;
}

ff::ValuePtr ff::SavedDictValueType::Load(ff::IDataReader* stream) const
{
	return ff::SavedDataValueType::Load(stream, true);
}

bool ff::SavedDictValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	return value->Convert<SavedDataValue>()->Save(stream);
}

ff::ComPtr<IUnknown> ff::SavedDictValueType::GetComObject(const ff::Value* value) const
{
	return value->GetValue<SavedDictValue>().Interface();
}

ff::String ff::SavedDictValueType::Print(const ff::Value* value)
{
	ff::StaticString name(L"SavedDict");
	return ff::SavedDataValueType::Print(value, name);
}
