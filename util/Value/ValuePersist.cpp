#include "pch.h"
#include "Data/DataPersist.h"
#include "Globals/Log.h"
#include "String/StringUtil.h"
#include "Value/ValuePersist.h"
#include "Value/Values.h"

static ff::String SanitizeString(ff::StringRef str)
{
	ff::String cleanStr = str;

	ff::ReplaceAll(cleanStr, ff::String(L"\r"), ff::String(L"\\r"));
	ff::ReplaceAll(cleanStr, ff::String(L"\n"), ff::String(L"\\n"));

	if (cleanStr.size() > 80)
	{
		cleanStr = cleanStr.substr(0, 40) + ff::String(L"...") + cleanStr.substr(cleanStr.size() - 40, 40);
	}

	return cleanStr;
}

static void InternalDumpValue(ff::StringRef name, ff::ValuePtr value, ff::Log& log, size_t level)
{
	assertRet(value);

	ff::String spaces(level * 4, L' ');

	if (name.size())
	{
		log.TraceF(L"%s%s: ", spaces.c_str(), name.c_str());
	}

	ff::String str = ::SanitizeString(value->Print());
	log.TraceF(L"%s\r\n", str.c_str());

	for (ff::String childName : value->GetChildNames(true))
	{
		InternalDumpValue(childName, value->GetNamedChild(childName), log, level + 1);
	}

	for (size_t i = 0; i < value->GetIndexChildCount(); i++)
	{
		log.TraceF(L"%s    [%lu] ", spaces.c_str(), i);
		InternalDumpValue(ff::GetEmptyString(), value->GetIndexChild(i), log, level + 2);
	}

	if (!value->CanHaveIndexedChildren() && !value->CanHaveNamedChildren())
	{
		ff::ValuePtr dictValue = value->Convert<ff::DictValue>();
		if (dictValue)
		{
			for (ff::String childName : dictValue->GetChildNames(true))
			{
				InternalDumpValue(childName, dictValue->GetNamedChild(childName), log, level + 1);
			}
		}
	}
}

void ff::DumpValue(ff::StringRef name, const Value* value, ff::Log* log, bool debugOnly)
{
	if (!debugOnly || ff::GetThisModule().IsDebugBuild())
	{
		ff::Log extraLog;
		ff::Log& realLog = log ? *log : extraLog;
		InternalDumpValue(name, value, realLog, 0);
	}
}

void ff::DebugDumpValue(const Value* value)
{
	ff::DumpValue(ff::GetEmptyString(), value, nullptr, true);
}

bool ff::SaveTypedValue(ff::IDataWriter* stream, const ff::Value* value)
{
	assertRetVal(ff::SaveData(stream, value->GetTypeId()), false);
	assertRetVal(value->Save(stream), false);
	return true;
}

ff::ValuePtr ff::LoadTypedValue(ff::IDataReader* stream)
{
	DWORD type;
	assertRetVal(ff::LoadData(stream, type), false);
	return ff::Value::Load(type, stream);
}
