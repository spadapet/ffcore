#include "pch.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Dict/DictPersist.h"
#include "Value/ValuePersist.h"
#include "Value/Values.h"

bool ff::SaveDict(const Dict& dict, ff::IDataWriter* writer)
{
	ff::Vector<ff::String> names = dict.GetAllNames();
	DWORD count = (DWORD)names.Size();
	assertRetVal(ff::SaveData(writer, count), false);

	for (ff::StringRef name : names)
	{
		assertRetVal(ff::SaveData(writer, name), false);

		ff::ValuePtr value = dict.GetValue(name);
		assertRetVal(ff::SaveTypedValue(writer, value), false);
	}

	return true;
}

ff::ComPtr<ff::IData> ff::SaveDict(const ff::Dict& dict)
{
	ff::ComPtr<ff::IDataVector> data;
	ff::ComPtr<ff::IDataWriter> writer;
	assertRetVal(ff::CreateDataWriter(&data, &writer), nullptr);
	return ff::SaveDict(dict, writer) ? data.Interface() : nullptr;
}

bool ff::LoadDict(ff::IDataReader* reader, ff::Dict& dict)
{
	DWORD count = 0;
	assertRetVal(ff::LoadData(reader, count), false);
	dict.Reserve(count);

	for (size_t i = 0; i < count; i++)
	{
		ff::String name;
		assertRetVal(ff::LoadData(reader, name), false);

		ff::ValuePtr value = ff::LoadTypedValue(reader);
		assertRetVal(value, false);

		dict.SetValue(name, value);
	}

	return true;
}

void ff::DumpDict(ff::StringRef name, const Dict& dict, ff::Log* log, bool debugOnly)
{
	if (!debugOnly || ff::GetThisModule().IsDebugBuild())
	{
		ff::DumpValue(name, ff::Value::New<DictValue>(Dict(dict)), log, debugOnly);
	}
}

void ff::DebugDumpDict(const Dict& dict)
{
	ff::DumpDict(ff::GetEmptyString(), dict, nullptr, true);
}
