#include "pch.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Value/Values.h"

ff::DataValue::DataValue(ff::IData* value)
	: _value(value)
{
}

ff::DataValue::DataValue(const void* data, size_t size)
{
	ff::Vector<BYTE> vec;
	vec.Push((const BYTE*)data, size);
	_value = ff::CreateDataVector(std::move(vec));
}

const ff::ComPtr<ff::IData>& ff::DataValue::GetValue() const
{
	return _value;
}

ff::Value* ff::DataValue::GetStaticValue(ff::IData* value)
{
	return !value ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::DataValue::GetStaticValue(const void* data, size_t size)
{
	return !data ? GetStaticValue(nullptr) : nullptr;
}

ff::Value* ff::DataValue::GetStaticDefaultValue()
{
	static DataValue s_null(nullptr);
	return &s_null;
}

ff::ValuePtr ff::DataValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::ComPtr<ff::IData>& src = value->GetValue<DataValue>();

	if (type == typeid(SavedDataValue) && src)
	{
		ff::ComPtr<ff::ISavedData> savedData;
		if (ff::CreateLoadedDataFromMemory(src, false, &savedData))
		{
			return ff::Value::New<SavedDataValue>(savedData);
		}
	}

	return nullptr;
}

ff::ValuePtr ff::DataValueType::Load(ff::IDataReader* stream) const
{
	return ff::SavedDataValueType::Load(stream, false);
}

bool ff::DataValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	return value->Convert<SavedDataValue>()->Save(stream);
}

ff::ComPtr<IUnknown> ff::DataValueType::GetComObject(const ff::Value* value) const
{
	return value->GetValue<DataValue>().Interface();
}

ff::String ff::DataValueType::Print(const ff::Value* value)
{
	ff::ComPtr<ff::IData> data = value->GetValue<DataValue>();
	return ff::String::format_new(L"<Data: %lu>", data ? data->GetSize() : 0);
}
