#include "pch.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"

bool ff::SaveBytes(IDataWriter* pWriter, const void* pMem, size_t nBytes)
{
	assertRetVal(pWriter, false);
	assertRetVal(pWriter->Write(pMem, nBytes), false);

	// Make sure that the data size is a multiple of 32 bits
	// STATIC_DATA (pod)
	static const BYTE padding[4] = { 0, 0, 0, 0 };
	size_t nPadding = ff::RoundUp(nBytes, 4) - nBytes;

	if (nPadding)
	{
		assertRetVal(pWriter->Write(padding, nPadding), false);
	}

	return true;
}

bool ff::SaveBytes(IDataWriter* pWriter, const IData* pData)
{
	assertRetVal(pData, false);
	noAssertRetVal(pData->GetSize(), true);

	return SaveBytes(pWriter, pData->GetMem(), pData->GetSize());
}

bool ff::LoadBytes(IDataReader* pReader, void* pMem, size_t nBytes)
{
	const BYTE* pData = LoadBytes(pReader, nBytes);
	assertRetVal(pData && pMem, false);

	CopyMemory(pMem, pData, nBytes);

	return true;
}

bool ff::LoadBytes(IDataReader* pReader, size_t nBytes, IData** ppData)
{
	assertRetVal(pReader, false);

	ComPtr<IData> pData;
	size_t nNewPos = std::min(pReader->GetSize(), pReader->GetPos() + ff::RoundUp(nBytes, 4));

	assertRetVal(pReader->Read(nBytes, ppData ? &pData : nullptr), false);
	assertRetVal(pReader->SetPos(nNewPos), false);

	if (ppData)
	{
		*ppData = pData.Detach();
	}

	return true;
}

const BYTE* ff::LoadBytes(IDataReader* pReader, size_t nBytes)
{
	assertRetVal(pReader, nullptr);

	return pReader->Read(ff::RoundUp(nBytes, 4));
}

template<>
bool ff::SaveData<ff::String>(ff::IDataWriter* pWriter, ff::StringRef data)
{
	DWORD nBytes = (DWORD)((data.length() + 1) * sizeof(wchar_t));

	assertRetVal(SaveData(pWriter, nBytes), false);

	return SaveBytes(pWriter, data.c_str(), nBytes);
}

template<>
bool ff::LoadData<ff::String>(IDataReader* pReader, StringOut data)
{
	DWORD nBytes = 0;
	assertRetVal(LoadData(pReader, nBytes), false);

	const BYTE* sz = LoadBytes(pReader, (size_t)nBytes);
	assertRetVal(sz, false);
	data.assign((const wchar_t*)sz, (nBytes / sizeof(wchar_t)) - 1);

	return true;
}
