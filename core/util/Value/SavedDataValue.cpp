#include "pch.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Value/Values.h"

ff::SavedDataValue::SavedDataValue(ff::ISavedData* value)
	: _value(value)
{
}

const ff::ComPtr<ff::ISavedData>& ff::SavedDataValue::GetValue() const
{
	return _value;
}

ff::Value* ff::SavedDataValue::GetStaticValue(ff::ISavedData* value)
{
	return !value ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::SavedDataValue::GetStaticDefaultValue()
{
	static SavedDataValue s_null(nullptr);
	return &s_null;
}

ff::ValuePtr ff::SavedDataValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::ComPtr<ff::ISavedData>& src = value->GetValue<SavedDataValue>();

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

	return nullptr;
}

ff::ValuePtr ff::SavedDataValueType::Load(ff::IDataReader* stream) const
{
	return ff::SavedDataValueType::Load(stream, false);
}

bool ff::SavedDataValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	DWORD valueSavedSize = 0;
	DWORD valueFullSize = ff::INVALID_DWORD;
	ff::ComPtr<ff::ISavedData> savedData = value->GetValue<SavedDataValue>();

	if (!savedData)
	{
		assertRetVal(ff::SaveData(stream, valueSavedSize), false);
		assertRetVal(ff::SaveData(stream, valueFullSize), false);
		return true;
	}

	ff::ComPtr<ff::ISavedData> cloneData;
	assertRetVal(savedData->Clone(&cloneData), false);

	ff::ComPtr<ff::IData> data = cloneData->SaveToMem();
	assertRetVal(data, false);

	valueSavedSize = (DWORD)data->GetSize();
	valueFullSize = cloneData->IsCompressed() ? (DWORD)cloneData->GetFullSize() : ff::INVALID_DWORD;

	assertRetVal(ff::SaveData(stream, valueSavedSize), false);
	assertRetVal(ff::SaveData(stream, valueFullSize), false);
	assertRetVal(ff::SaveBytes(stream, data), false);

	return true;
}

ff::ComPtr<IUnknown> ff::SavedDataValueType::GetComObject(const ff::Value* value) const
{
	return value->GetValue<SavedDataValue>().Interface();
}

ff::String ff::SavedDataValueType::Print(const ff::Value* value)
{
	ff::StaticString name(L"SavedData");
	return ff::SavedDataValueType::Print(value, name);
}

ff::ValuePtr ff::SavedDataValueType::Load(ff::IDataReader* stream, bool savedDict)
{
	DWORD savedSize = 0;
	DWORD fullSize = 0;
	assertRetVal(ff::LoadData(stream, savedSize), false);
	assertRetVal(ff::LoadData(stream, fullSize), false);

	bool compressed = (fullSize != ff::INVALID_DWORD);
	fullSize = compressed ? fullSize : savedSize;

	ff::ComPtr<ff::ISavedData> savedData;
	assertRetVal(stream->CreateSavedData(stream->GetPos(), savedSize, fullSize, compressed, &savedData), false);
	assertRetVal(ff::LoadBytes(stream, savedSize, nullptr), false);

	return savedDict
		? ff::Value::New<SavedDictValue>(savedData)
		: ff::Value::New<SavedDataValue>(savedData);
}

ff::String ff::SavedDataValueType::Print(const ff::Value* value, ff::StringRef name)
{
	size_t savedSize = 0;
	size_t fullSize = 0;
	bool compressed = false;

	ff::ComPtr<ff::ISavedData> data = value->Convert<SavedDataValue>()->GetValue<SavedDataValue>();
	ff::ComPtr<ff::ISavedData> dataClone;
	if (data && data->Clone(&dataClone))
	{
		savedSize = dataClone->GetSavedSize();
		fullSize = dataClone->GetFullSize();
		compressed = dataClone->IsCompressed();
	}

	if (compressed && fullSize)
	{
		double savedSizeD = (double)savedSize;
		double fullSizeD = (double)fullSize;

		return ff::String::format_new(L"<%s: %lu -> %lu (%.1f%%)>", name.c_str(), savedSize, fullSize, (fullSizeD - savedSizeD) * 100.0 / fullSizeD);
	}
	else
	{
		return ff::String::format_new(L"<%s: %lu>", name.c_str(), fullSize);
	}
}
