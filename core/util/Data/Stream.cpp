#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Data/Stream.h"
#include "Globals/Log.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Value/Values.h"

class __declspec(uuid("6d01bbbf-9e9c-46d3-ba84-c7fba920486e"))
	SavedDataStream : public ff::ComBase, public IStream
{
public:
	DECLARE_HEADER(SavedDataStream);

	bool Init(ff::IData* pReadData, ff::IDataVector* pWriteData, size_t nPos);

	// IStream functions

	COM_FUNC Read(void* pv, ULONG cb, ULONG* pcbRead) override;
	COM_FUNC Write(const void* pv, ULONG cb, ULONG* pcbWritten) override;
	COM_FUNC Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) override;
	COM_FUNC SetSize(ULARGE_INTEGER libNewSize) override;
	COM_FUNC CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) override;
	COM_FUNC Commit(DWORD grfCommitFlags) override;
	COM_FUNC Revert() override;
	COM_FUNC LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
	COM_FUNC UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
	COM_FUNC Stat(STATSTG* pstatstg, DWORD grfStatFlag) override;
	COM_FUNC Clone(IStream** ppstm) override;

protected:
	const BYTE* Data();
	LPBYTE WritableData();
	size_t Size();

	ff::ComPtr<ff::IData> _readData;
	ff::ComPtr<ff::IDataVector> _writeData;
	size_t _pos;
};

BEGIN_INTERFACES(SavedDataStream)
	HAS_INTERFACE(IStream)
END_INTERFACES()

class __declspec(uuid("1a446cf8-07a8-4913-adfe-7368c4c7d706"))
	DataReaderStream : public ff::ComBase, public IStream
{
public:
	DECLARE_HEADER(DataReaderStream);

	bool Init(ff::IDataReader* reader);

	// IStream functions

	COM_FUNC Read(void* pv, ULONG cb, ULONG* pcbRead) override;
	COM_FUNC Write(const void* pv, ULONG cb, ULONG* pcbWritten) override;
	COM_FUNC Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) override;
	COM_FUNC SetSize(ULARGE_INTEGER libNewSize) override;
	COM_FUNC CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) override;
	COM_FUNC Commit(DWORD grfCommitFlags) override;
	COM_FUNC Revert() override;
	COM_FUNC LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
	COM_FUNC UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
	COM_FUNC Stat(STATSTG* pstatstg, DWORD grfStatFlag) override;
	COM_FUNC Clone(IStream** ppstm) override;

protected:
	ff::ComPtr<ff::IDataReader> _reader;
};

BEGIN_INTERFACES(DataReaderStream)
	HAS_INTERFACE(IStream)
END_INTERFACES()

bool ff::CreateWriteStream(ff::IDataVector** ppData, IStream** ppStream)
{
	ff::ComPtr<ff::IDataVector> pDataVector;
	assertRetVal(CreateDataVector(0, &pDataVector), false);

	if (ppData)
	{
		*ppData = GetAddRef<ff::IDataVector>(pDataVector);
	}

	return CreateWriteStream(pDataVector, 0, ppStream);
}

bool ff::CreateWriteStream(ff::IDataVector* pData, size_t nPos, IStream** ppStream)
{
	assertRetVal(ppStream, false);
	*ppStream = nullptr;

	ff::ComPtr<SavedDataStream> pStream = new ComObject<SavedDataStream>;
	assertRetVal(pStream->Init(nullptr, pData, nPos), false);

	*ppStream = pStream.Detach();
	return true;
}

bool ff::CreateReadStream(ff::IData* pData, size_t nPos, IStream** ppStream)
{
	assertRetVal(ppStream, false);
	*ppStream = nullptr;

	ff::ComPtr<SavedDataStream> pStream = new ComObject<SavedDataStream>;
	assertRetVal(pStream->Init(pData, nullptr, nPos), false);

	*ppStream = pStream.Detach();
	return true;
}

bool ff::CreateReadStream(ff::IDataReader* reader, IStream** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ComPtr<DataReaderStream> myObj = new ComObject<DataReaderStream>;
	assertRetVal(myObj->Init(reader), false);

	*obj = myObj.Detach();
	return true;
}

bool ff::CreateReadStream(ff::IDataReader* reader, IMFByteStream** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

#if METRO_APP
	// Use the Windows stream implementation when possible
	{
		Windows::Storage::Streams::IRandomAccessStream^ stream = ff::GetRandomAccessStream(reader);
		if (stream && SUCCEEDED(MFCreateMFByteStreamOnStreamEx((IUnknown*)stream, obj)))
		{
			return true;
		}
	}

	// Use IStream
	{
		ff::ComPtr<IStream> stream;
		assertRetVal(ff::CreateReadStream(reader, &stream), false);
		assertHrRetVal(MFCreateMFByteStreamOnStreamEx(stream, obj), false);
	}
#else
	// Use IStream
	{
		ff::ComPtr<IStream> stream;
		assertRetVal(ff::CreateReadStream(reader, &stream), false);
		assertHrRetVal(MFCreateMFByteStreamOnStream(stream, obj), false);
	}
#endif

	return true;
}

SavedDataStream::SavedDataStream()
	: _pos(0)
{
}

SavedDataStream::~SavedDataStream()
{
}

bool SavedDataStream::Init(ff::IData* pReadData, ff::IDataVector* pWriteData, size_t nPos)
{
	_readData = pReadData;
	_writeData = pWriteData;
	_pos = nPos;

	if (!_readData && !_writeData)
	{
		assertRetVal(CreateDataVector(0, &_writeData), false);
	}

	return true;
}

const BYTE* SavedDataStream::Data()
{
	if (_readData)
	{
		return _readData->GetMem();
	}
	else if (_writeData)
	{
		return _writeData->GetMem();
	}
	else
	{
		assertRetVal(false, nullptr);
	}
}

LPBYTE SavedDataStream::WritableData()
{
	if (_writeData)
	{
		return _writeData->GetVector().Data();
	}
	else
	{
		assertRetVal(false, nullptr);
	}
}

size_t SavedDataStream::Size()
{
	if (_readData)
	{
		return _readData->GetSize();
	}
	else if (_writeData)
	{
		return _writeData->GetSize();
	}
	else
	{
		assertRetVal(false, 0);
	}
}

HRESULT SavedDataStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
	assertRetVal(Data(), E_FAIL);

	size_t nRead = 0;

	if (_pos < Size())
	{
		nRead = std::min((size_t)cb, Size() - _pos);
	}

	if (pcbRead)
	{
		*pcbRead = (ULONG)nRead;
	}

	if (nRead)
	{
		CopyMemory(pv, Data() + _pos, nRead);
		_pos += nRead;
	}

	return (nRead == (size_t)cb) ? S_OK : S_FALSE;
}

HRESULT SavedDataStream::Write(const void* pv, ULONG cb, ULONG* pcbWritten)
{
	assertRetVal(_writeData, STG_E_CANTSAVE);
	assertRetVal(pv, STG_E_INVALIDPOINTER);

	size_t nWrite = (size_t)cb;

	if (_pos + nWrite > Size())
	{
		_writeData->GetVector().Resize(_pos + nWrite);
	}

	assertRetVal(Size() >= _pos + nWrite, STG_E_MEDIUMFULL);

	if (pcbWritten)
	{
		*pcbWritten = (ULONG)nWrite;
	}

	if (nWrite)
	{
		CopyMemory(WritableData() + _pos, pv, nWrite);
		_pos += nWrite;
	}

	return S_OK;
}

HRESULT SavedDataStream::Seek(
	LARGE_INTEGER dlibMove,
	DWORD dwOrigin,
	ULARGE_INTEGER* plibNewPosition)
{
	assertRetVal(Data(), STG_E_INVALIDFUNCTION);

	LARGE_INTEGER nSize;
	nSize.QuadPart = (LONGLONG)Size();

	LARGE_INTEGER nNewPos;
	nNewPos.QuadPart = (LONGLONG)_pos;

	if (nNewPos.QuadPart < 0 || nSize.QuadPart < 0)
	{
		assertRetVal(false, STG_E_INVALIDFUNCTION);
	}

	if (plibNewPosition)
	{
		*plibNewPosition = *(ULARGE_INTEGER*)&nNewPos;
	}

	switch (dwOrigin)
	{
	case STREAM_SEEK_SET:
		nNewPos = dlibMove;
		break;

	case STREAM_SEEK_CUR:
		nNewPos.QuadPart += dlibMove.QuadPart;
		break;

	case STREAM_SEEK_END:
		nNewPos.QuadPart = nSize.QuadPart + dlibMove.QuadPart;
		break;

	default:
		return STG_E_INVALIDFUNCTION;
	}

	if (nNewPos.QuadPart < 0)
	{
		assertRetVal(false, STG_E_INVALIDFUNCTION);
	}

	if (plibNewPosition)
	{
		*plibNewPosition = *(ULARGE_INTEGER*)&nNewPos;
	}

	_pos = (size_t)nNewPos.QuadPart;

	return S_OK;
}

HRESULT SavedDataStream::SetSize(ULARGE_INTEGER libNewSize)
{
	assertRetVal(_writeData, STG_E_INVALIDFUNCTION);

	size_t nNewSize = (size_t)libNewSize.QuadPart;

	_writeData->GetVector().Resize(nNewSize);

	return S_OK;
}

HRESULT SavedDataStream::CopyTo(
	IStream* pstm,
	ULARGE_INTEGER cb,
	ULARGE_INTEGER* pcbRead,
	ULARGE_INTEGER* pcbWritten)
{
	assertRetVal(pstm && pstm != this && Data(), STG_E_INVALIDPOINTER);

	size_t nRead = 0;

	if (_pos < Size())
	{
		nRead = std::min((size_t)cb.QuadPart, Size() - _pos);
	}

	if (pcbRead)
	{
		pcbRead->QuadPart = (ULONGLONG)nRead;
	}

	if (nRead)
	{
		ff::ComPtr<SavedDataStream> pMyDest;

		if (pMyDest.QueryFrom(pstm) && pMyDest->Data() == Data())
		{
			// copying within myself, so be safe and make a copy of the data first

			ff::Vector<BYTE> dataCopy;
			dataCopy.Resize(nRead);
			CopyMemory(dataCopy.Data(), Data() + _pos, nRead);

			ULONG nWritten = 0;
			HRESULT hr = pstm->Write(dataCopy.Data(), (ULONG)nRead, &nWritten);
			_pos += nRead;

			if (pcbWritten)
			{
				pcbWritten->QuadPart = (ULONGLONG)nWritten;
			}

			return hr;
		}
		else
		{
			ULONG nWritten = 0;
			HRESULT hr = pstm->Write(Data() + _pos, (ULONG)nRead, &nWritten);
			_pos += nRead;

			if (pcbWritten)
			{
				pcbWritten->QuadPart = (ULONGLONG)nWritten;
			}

			return hr;
		}
	}
	else
	{
		if (pcbWritten)
		{
			pcbWritten->QuadPart = 0;
		}

		return S_OK;
	}
}

HRESULT SavedDataStream::Commit(DWORD grfCommitFlags)
{
	return S_OK;
}

HRESULT SavedDataStream::Revert()
{
	return S_OK;
}

HRESULT SavedDataStream::LockRegion(
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT SavedDataStream::UnlockRegion(
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT SavedDataStream::Stat(STATSTG* pstatstg, DWORD grfStatFlag)
{
	assertRetVal(pstatstg && Data(), STG_E_INVALIDPOINTER);
	ff::ZeroObject(*pstatstg);

	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = (ULONGLONG)Size();

	return S_OK;
}

HRESULT SavedDataStream::Clone(IStream** ppstm)
{
	assertRetVal(ppstm, STG_E_INVALIDPOINTER);
	*ppstm = nullptr;

	ff::ComPtr<SavedDataStream> pClone = new ff::ComObject<SavedDataStream>;
	assertRetVal(pClone->Init(_readData, _writeData, _pos), E_FAIL);

	*ppstm = pClone.Detach();

	return S_OK;
}

DataReaderStream::DataReaderStream()
{
}

DataReaderStream::~DataReaderStream()
{
}

bool DataReaderStream::Init(ff::IDataReader* reader)
{
	assertRetVal(reader, false);
	_reader = reader;
	return true;
}

HRESULT DataReaderStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
	size_t bytesLeft = _reader->GetSize() - _reader->GetPos();
	size_t nRead = std::min((size_t)cb, bytesLeft);

	if (pcbRead)
	{
		*pcbRead = (ULONG)nRead;
	}

	if (nRead)
	{
		::CopyMemory(pv, _reader->Read(nRead), nRead);
	}

	return (nRead == (size_t)cb) ? S_OK : S_FALSE;
}

HRESULT DataReaderStream::Write(const void* pv, ULONG cb, ULONG* pcbWritten)
{
	assertRetVal(false, STG_E_CANTSAVE);
}

HRESULT DataReaderStream::Seek(
	LARGE_INTEGER dlibMove,
	DWORD dwOrigin,
	ULARGE_INTEGER* plibNewPosition)
{
	LARGE_INTEGER nSize;
	nSize.QuadPart = (LONGLONG)_reader->GetSize();

	LARGE_INTEGER nNewPos;
	nNewPos.QuadPart = (LONGLONG)_reader->GetPos();

	if (nNewPos.QuadPart < 0 || nSize.QuadPart < 0)
	{
		assertRetVal(false, STG_E_INVALIDFUNCTION);
	}

	if (plibNewPosition)
	{
		*plibNewPosition = *(ULARGE_INTEGER*)&nNewPos;
	}

	switch (dwOrigin)
	{
	case STREAM_SEEK_SET:
		nNewPos = dlibMove;
		break;

	case STREAM_SEEK_CUR:
		nNewPos.QuadPart += dlibMove.QuadPart;
		break;

	case STREAM_SEEK_END:
		nNewPos.QuadPart = nSize.QuadPart + dlibMove.QuadPart;
		break;

	default:
		return STG_E_INVALIDFUNCTION;
	}

	if (nNewPos.QuadPart < 0)
	{
		assertRetVal(false, STG_E_INVALIDFUNCTION);
	}

	if (plibNewPosition)
	{
		*plibNewPosition = *(ULARGE_INTEGER*)&nNewPos;
	}

	assertRetVal(_reader->SetPos((size_t)nNewPos.QuadPart), STG_E_INVALIDFUNCTION);

	return S_OK;
}

HRESULT DataReaderStream::SetSize(ULARGE_INTEGER libNewSize)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT DataReaderStream::CopyTo(
	IStream* pstm,
	ULARGE_INTEGER cb,
	ULARGE_INTEGER* pcbRead,
	ULARGE_INTEGER* pcbWritten)
{
	assertRetVal(pstm && pstm != this, STG_E_INVALIDPOINTER);

	size_t bytesLeft = _reader->GetSize() - _reader->GetPos();
	size_t nRead = std::min((size_t)cb.QuadPart, bytesLeft);

	if (pcbRead)
	{
		pcbRead->QuadPart = (ULONGLONG)nRead;
	}

	if (nRead)
	{
		ULONG nWritten = 0;
		HRESULT hr = pstm->Write(_reader->Read(nRead), (ULONG)nRead, &nWritten);

		if (pcbWritten)
		{
			pcbWritten->QuadPart = (ULONGLONG)nWritten;
		}

		return hr;
	}
	else
	{
		if (pcbWritten)
		{
			pcbWritten->QuadPart = 0;
		}

		return S_OK;
	}
}

HRESULT DataReaderStream::Commit(DWORD grfCommitFlags)
{
	return S_OK;
}

HRESULT DataReaderStream::Revert()
{
	return S_OK;
}

HRESULT DataReaderStream::LockRegion(
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT DataReaderStream::UnlockRegion(
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT DataReaderStream::Stat(STATSTG* pstatstg, DWORD grfStatFlag)
{
	assertRetVal(pstatstg, STG_E_INVALIDPOINTER);
	ZeroMemory(pstatstg, sizeof(*pstatstg));

	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = (ULONGLONG)_reader->GetSize();

	return S_OK;
}

HRESULT DataReaderStream::Clone(IStream** ppstm)
{
	assertRetVal(false, STG_E_INVALIDFUNCTION);
}

static ff::StaticString PROP_COMPRESS(L"compress");
static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString RES_COMPRESS(L"res:compress");

class __declspec(uuid("96d0d0d7-1ed7-456b-89cf-6d6641466a68"))
	FileStream
	: public ff::ComBase
	, public ff::IFileStream
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(FileStream);

	// IFileStream functions
	virtual bool CreateReader(ff::IDataReader** obj) override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	ff::ComPtr<ff::ISavedData> _data;
	bool _compress;
};

BEGIN_INTERFACES(FileStream)
	HAS_INTERFACE(ff::IFileStream)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup RegisterFileStream([](ff::Module& module)
	{
		module.RegisterClassT<FileStream>(L"file");
	});

FileStream::FileStream()
	: _compress(true)
{
}

FileStream::~FileStream()
{
}

bool FileStream::CreateReader(ff::IDataReader** obj)
{
	assertRetVal(obj && _data, false);
	return _data->CreateSavedDataReader(obj);
}

bool FileStream::LoadFromSource(const ff::Dict& dict)
{
	_compress = dict.Get<ff::BoolValue>(PROP_COMPRESS, true);

	ff::ComPtr<ff::IDataFile> file;
	ff::String fullFile = dict.Get<ff::StringValue>(PROP_FILE);
	assertRetVal(ff::CreateDataFile(fullFile, false, &file), false);
	assertRetVal(ff::CreateSavedDataFromFile(file, 0, file->GetSize(), file->GetSize(), false, &_data), false);

	return true;
}

bool FileStream::LoadFromCache(const ff::Dict& dict)
{
	_compress = dict.Get<ff::BoolValue>(PROP_COMPRESS, true);
	_data = dict.Get<ff::SavedDataValue>(PROP_DATA);
	assertRetVal(_data, false);

	return true;
}

bool FileStream::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::SavedDataValue>(PROP_DATA, _data);
	dict.Set<ff::BoolValue>(PROP_COMPRESS, _compress);
	dict.Set<ff::BoolValue>(RES_COMPRESS, _compress);

	return true;
}
