#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Compression.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"

class __declspec(uuid("98514703-8655-49f8-8c2e-13641af6dd9c"))
	SavedData : public ff::ComBase, public ff::ISavedData
{
public:
	DECLARE_HEADER(SavedData);

	bool CreateLoadedDataFromMemory(ff::IData* pData, bool bCompress);
	bool CreateSavedDataFromMemory(ff::IData* pData, size_t nFullSize, bool bCompressed);
	bool CreateSavedDataFromFile(ff::IDataFile* pFile, size_t nStart, size_t nSavedSize, size_t nFullSize, bool bCompressed);

	// ISavedData functions

	virtual ff::IData* Load() override;
	virtual bool Unload() override;
	virtual ff::IData* SaveToMem() override;
	virtual bool SaveToFile() override;

	virtual size_t GetSavedSize() override;
	virtual size_t GetFullSize() const override;
	virtual bool IsCompressed() const override;
	virtual bool CreateSavedDataReader(ff::IDataReader** ppReader) override;

	virtual bool Clone(ff::ISavedData** ppSavedData) const override;
	virtual bool Copy(const ff::ISavedData* pDataSource) override;

private:
	ff::ComPtr<ff::IData> _fullData;
	ff::ComPtr<ff::IData> _origData;
	size_t _origDataFullSize;
	bool _compress;

	ff::ComPtr<ff::IDataFile> _origFile;
	size_t _origFileStart;
	size_t _origFileSize;
	size_t _origFileFullSize;
};

BEGIN_INTERFACES(SavedData)
	HAS_INTERFACE(ff::ISavedData)
END_INTERFACES()

bool ff::CreateLoadedDataFromMemory(ff::IData* pData, bool bCompress, ff::ISavedData** ppSavedData)
{
	assertRetVal(ppSavedData, false);
	*ppSavedData = nullptr;

	ff::ComPtr<SavedData, ff::ISavedData> pSavedData = new ComObject<SavedData>;
	assertRetVal(pSavedData->CreateLoadedDataFromMemory(pData, bCompress), false);

	*ppSavedData = pSavedData.Detach();

	return true;
}

bool ff::CreateSavedDataFromMemory(ff::IData* pData, size_t nFullSize, bool bCompressed, ff::ISavedData** ppSavedData)
{
	assertRetVal(ppSavedData, false);
	*ppSavedData = nullptr;

	ff::ComPtr<SavedData, ff::ISavedData> pSavedData = new ComObject<SavedData>;
	if (bCompressed)
	{
		assertRetVal(pSavedData->CreateSavedDataFromMemory(pData, nFullSize, bCompressed), false);
	}
	else
	{
		assertRetVal(nFullSize == pData->GetSize(), false);
		assertRetVal(pSavedData->CreateLoadedDataFromMemory(pData, false), false);
	}

	*ppSavedData = pSavedData.Detach();

	return true;
}

bool ff::CreateSavedDataFromFile(IDataFile* pFile, size_t nStart, size_t nSavedSize, size_t nFullSize, bool bCompressed, ISavedData** ppSavedData)
{
	assertRetVal(ppSavedData, false);
	*ppSavedData = nullptr;

	if (pFile && pFile->GetMem())
	{
		ff::ComPtr<ff::IData> pData;

		if (!CreateDataInMemMappedFile(pFile->GetMem() + nStart, nSavedSize, pFile, &pData) ||
			!CreateSavedDataFromMemory(pData, nFullSize, bCompressed, ppSavedData))
		{
			assertRetVal(false, false);
		}
	}
	else
	{
		ff::ComPtr<SavedData, ff::ISavedData> pSavedData = new ComObject<SavedData>;

		assertRetVal(pSavedData->CreateSavedDataFromFile(pFile, nStart, nSavedSize, nFullSize, bCompressed), false);

		*ppSavedData = pSavedData.Detach();
	}

	return true;
}

SavedData::SavedData()
	: _compress(false)
	, _origDataFullSize(0)
	, _origFileStart(0)
	, _origFileSize(0)
	, _origFileFullSize(0)
{
}

SavedData::~SavedData()
{
}

bool SavedData::CreateLoadedDataFromMemory(ff::IData* pData, bool bCompress)
{
	assertRetVal(pData, false);

	_fullData = pData;
	_compress = bCompress;

	if (!bCompress)
	{
		_origData = pData;
		_origDataFullSize = pData->GetSize();
	}

	return true;
}

bool SavedData::CreateSavedDataFromMemory(ff::IData* pData, size_t nFullSize, bool bCompressed)
{
	assertRetVal(pData, false);

	if (!bCompressed)
	{
		assertRetVal(nFullSize == pData->GetSize(), false);
		return CreateLoadedDataFromMemory(pData, bCompressed);
	}
	else
	{
		_origData = pData;
		_origDataFullSize = nFullSize;
		_compress = bCompressed;
	}

	return true;
}

bool SavedData::CreateSavedDataFromFile(ff::IDataFile* pFile, size_t nStart, size_t nSavedSize, size_t nFullSize, bool bCompressed)
{
	assertRetVal(pFile, false);
	assertSz(!pFile->GetMem(), L"Can't use a mem mapped file in SavedData::CreateSavedDataFromFile");

	_origFile = pFile;
	_origFileStart = nStart;
	_origFileSize = nSavedSize;
	_origFileFullSize = nFullSize;
	_compress = bCompressed;

	return true;
}

ff::IData* SavedData::Load()
{
	if (!_fullData && !_compress && _origData)
	{
		_fullData = _origData;

		if (!_origData->IsStatic())
		{
			_origData = nullptr;
		}
	}

	if (!_fullData)
	{
		size_t nSavedSize = GetSavedSize();
		size_t nFullSize = GetFullSize();
		bool bSuccess = false;

		ff::ComPtr<ff::IDataReader> pReader;
		ff::ComPtr<ff::IDataWriter> pWriter;
		ff::ComPtr<ff::IDataVector> pDataVector;

		if (ff::CreateDataVector(nFullSize, &pDataVector) && ff::CreateDataWriter(pDataVector, 0, &pWriter))
		{
			if (_origFile)
			{
				bSuccess = ff::CreateDataReader(_origFile, _origFileStart, &pReader);
			}
			else if (_origData)
			{
				bSuccess = ff::CreateDataReader(_origData, 0, &pReader);
			}
		}

		if (bSuccess)
		{
			bSuccess = _compress
				? UncompressData(pReader, nSavedSize, pWriter)
				: StreamCopyData(pReader, nSavedSize, pWriter);

			assert(pWriter->GetPos() == nFullSize && pDataVector->GetVector().Size() == nFullSize);

			bSuccess = bSuccess && pWriter->GetPos() == nFullSize;
		}

		if (bSuccess)
		{
			_fullData = pDataVector;

			if (_origData && !_origData->IsStatic())
			{
				_origData = nullptr;
			}
		}
	}

	assert(_fullData);
	return _fullData;
}

bool SavedData::Unload()
{
	if (_origFile)
	{
		// still have the original file data, get rid of anything in memory

		if (_origData && !_origData->IsStatic())
		{
			_origData = nullptr;
		}
	}
	else if (_origData)
	{
		// still have the original saved data
	}
	else if (_fullData)
	{
		if (_compress)
		{
			CompressData(_fullData, &_origData);
		}
		else
		{
			_origData = _fullData;

		}

		_origDataFullSize = _fullData->GetSize();
	}

	_fullData = nullptr;

	return true;
}

ff::IData* SavedData::SaveToMem()
{
	if (!_origData)
	{
		if (_fullData)
		{
			if (_compress)
			{
				CompressData(_fullData, &_origData);
			}
			else
			{
				_origData = _fullData;
			}

			_origDataFullSize = _fullData->GetSize();
		}
		else if (_origFile)
		{
			// load a chunk of the file into memory

			ff::ComPtr<ff::IDataReader> pReader;
			ff::ComPtr<ff::IDataWriter> pWriter;
			ff::ComPtr<ff::IDataVector> pDataVector;

			if (ff::CreateDataVector(_origFileSize, &pDataVector) &&
				ff::CreateDataWriter(pDataVector, 0, &pWriter) &&
				ff::CreateDataReader(_origFile, _origFileStart, &pReader) &&
				ff::StreamCopyData(pReader, _origFileSize, pWriter))
			{
				_origData = pDataVector;
				_origDataFullSize = _origFileFullSize;
			}
		}
	}

	assertRetVal(_origData, nullptr);

	_fullData = nullptr;

	return _origData;
}

bool SavedData::SaveToFile()
{
	if (!_origFile && (!_origData || !_origData->IsStatic()))
	{
		ff::ComPtr<ff::IDataFile> pFile;
		ff::ComPtr<ff::IDataWriter> pWriter;
		ff::ComPtr<ff::IDataReader> pReader;

		assertRetVal(CreateTempDataFile(&pFile), false);
		assertRetVal(CreateDataWriter(pFile, 0, &pWriter), false);

		if (_origData)
		{
			assertRetVal(CreateDataReader(_origData, 0, &pReader), false);
			assertRetVal(StreamCopyData(pReader, _origData->GetSize(), pWriter), false);

			_origFile = pFile;
			_origFileStart = 0;
			_origFileSize = _origData->GetSize();
			_origFileFullSize = _origDataFullSize;
			_origData = nullptr;
		}
		else if (_fullData)
		{
			assertRetVal(CreateDataReader(_fullData, 0, &pReader), false);

			if (_compress)
			{
				assertRetVal(CompressData(pReader, _fullData->GetSize(), pWriter), false);
			}
			else
			{
				assertRetVal(StreamCopyData(pReader, _fullData->GetSize(), pWriter), false);
			}

			_origFile = pFile;
			_origFileStart = 0;
			_origFileSize = pWriter->GetPos();
			_origFileFullSize = _fullData->GetSize();
			_origData = nullptr;
		}
	}

	assertRetVal(_origFile || _origData, false);

	_fullData = nullptr;

	return true;
}

size_t SavedData::GetSavedSize()
{
	if (_origData)
	{
		return _origData->GetSize();
	}
	else if (_origFile)
	{
		return _origFileSize;
	}
	else if (SaveToMem())
	{
		return SaveToMem()->GetSize();
	}

	assertRetVal(false, 0);
}

size_t SavedData::GetFullSize() const
{
	if (_fullData)
	{
		return _fullData->GetSize();
	}
	else if (_origData)
	{
		return _origDataFullSize;
	}
	else if (_origFile)
	{
		return _origFileFullSize;
	}

	assertRetVal(false, 0);
}

bool SavedData::IsCompressed() const
{
	return _compress;
}

bool SavedData::CreateSavedDataReader(ff::IDataReader** ppReader)
{
	assertRetVal(ppReader, false);
	*ppReader = nullptr;

	if (!_origFile && !_origData)
	{
		assertRetVal(Unload(), false);
	}

	if (_origData)
	{
		assertRetVal(CreateDataReader(_origData, 0, ppReader), false);
	}
	else if (_origFile)
	{
		assertRetVal(CreateDataReader(_origFile, _origFileStart, ppReader), false);
	}

	assert(*ppReader);
	return *ppReader != nullptr;
}

bool SavedData::Clone(ff::ISavedData** ppSavedData) const
{
	assertRetVal(ppSavedData, false);
	*ppSavedData = nullptr;

	ff::ComPtr<SavedData, ff::ISavedData> pSavedData = new ff::ComObject<SavedData>;
	assertRetVal(pSavedData->Copy(this), false);

	*ppSavedData = pSavedData.Detach();

	return true;
}

bool SavedData::Copy(const ff::ISavedData* pDataSource)
{
	ff::ComPtr<SavedData, ff::ISavedData> pRealSource;
	assertRetVal(pRealSource.QueryFrom(pDataSource), false);

	_fullData = pRealSource->_fullData;
	_compress = pRealSource->_compress;

	_origData = pRealSource->_origData;
	_origDataFullSize = pRealSource->_origDataFullSize;

	_origFile = pRealSource->_origFile;
	_origFileStart = pRealSource->_origFileStart;
	_origFileSize = pRealSource->_origFileSize;
	_origFileFullSize = pRealSource->_origFileFullSize;

	return true;
}
