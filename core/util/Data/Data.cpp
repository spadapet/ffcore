#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Data.h"
#include "Data/DataFile.h"

#if METRO_APP
#include <robuffer.h>
#endif

class __declspec(uuid("d16151be-46ad-46c8-9441-8e26cd6cf136"))
	Data : public ff::ComBase, public ff::IData
{
public:
	DECLARE_HEADER(Data);

	bool Init(const BYTE* pMem, size_t nSize, ff::IDataFile* pFile);

	// IData functions
	virtual const BYTE* GetMem() const override;
	virtual size_t GetSize() const override;
	virtual ff::IDataFile* GetFile() const override;
	virtual bool IsStatic() const override;

private:
	ff::ComPtr<ff::IDataFile> _file;
	const BYTE* _mem;
	size_t _size;
};

BEGIN_INTERFACES(Data)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

class __declspec(uuid("f6b6f0a6-c7b5-468f-9042-62e81a1eeba5"))
	ChildData : public ff::ComBase, public ff::IData
{
public:
	DECLARE_HEADER(ChildData);

	bool Init(ff::IData* pParentData, size_t nPos, size_t nSize);

	// IData functions
	virtual const BYTE* GetMem() const override;
	virtual size_t GetSize() const override;
	virtual ff::IDataFile* GetFile() const override;
	virtual bool IsStatic() const override;

private:
	ff::ComPtr<ff::IData> _parentData;
	size_t _pos;
	size_t _size;
};

BEGIN_INTERFACES(ChildData)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

class __declspec(uuid("630c0577-9d72-44ed-85e0-c16e3d1e7664"))
	DataVector : public ff::ComBase, public ff::IDataVector
{
public:
	DECLARE_HEADER(DataVector);

	bool Init(size_t nInitialSize);
	void Init(ff::Vector<BYTE>&& data);

	// IData functions
	virtual const BYTE* GetMem() const override;
	virtual size_t GetSize() const override;
	virtual ff::IDataFile* GetFile() const override;
	virtual bool IsStatic() const override;

	// IDataVector functions
	virtual ff::Vector<BYTE>& GetVector() override;

private:
	ff::Vector<BYTE> _data;
};

BEGIN_INTERFACES(DataVector)
	HAS_INTERFACE(ff::IData)
	HAS_INTERFACE(ff::IDataVector)
END_INTERFACES()

#if METRO_APP

class __declspec(uuid("3d8c0f33-4d02-46b8-80f8-ce87548349f8"))
	DataBuffer : public ff::ComBase, public ff::IData
{
public:
	DECLARE_HEADER(DataBuffer);

	bool Init(Windows::Storage::Streams::IBuffer^ buffer);

	// IData functions
	virtual const BYTE* GetMem() const override;
	virtual size_t GetSize() const override;
	virtual ff::IDataFile* GetFile() const override;
	virtual bool IsStatic() const override;

private:
	Windows::Storage::Streams::IBuffer^ _buffer;
	const BYTE* _data;
};

BEGIN_INTERFACES(DataBuffer)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

#endif

bool ff::CreateDataInMemMappedFile(ff::IDataFile* pFile, ff::IData** ppData)
{
	return CreateDataInMemMappedFile(nullptr, ff::INVALID_SIZE, pFile, ppData);
}

bool ff::CreateDataInMemMappedFile(const BYTE* pMem, size_t nSize, ff::IDataFile* pFile, ff::IData** ppData)
{
	assertRetVal(ppData, false);
	*ppData = nullptr;

	ff::ComPtr<Data> pData = new ComObject<Data>;
	assertRetVal(pData->Init(pMem, nSize, pFile), false);

	*ppData = pData.Detach();

	return true;
}

bool ff::CreateDataInStaticMem(const BYTE* pMem, size_t nSize, ff::IData** ppData)
{
	return CreateDataInMemMappedFile(pMem, nSize, nullptr, ppData);
}

#if !METRO_APP
bool ff::CreateDataInResource(HINSTANCE hInstance, UINT id, ff::IData** ppData)
{
	return ff::CreateDataInResource(hInstance, MAKEINTRESOURCE(id), RT_RCDATA, ppData);
}

bool ff::CreateDataInResource(HINSTANCE hInstance, LPCWSTR type, LPCWSTR name, ff::IData** ppData)
{
	assertRetVal(ppData, false);
	*ppData = nullptr;

	assertRetVal(hInstance, false);

	HRSRC hFound = ::FindResourceW(hInstance, name, type);
	HGLOBAL hRes = hFound ? ::LoadResource(hInstance, hFound) : nullptr;
	const BYTE* pMem = hRes ? (LPBYTE)::LockResource(hRes) : nullptr;
	size_t nSize = pMem ? ::SizeofResource(hInstance, hFound) : 0;

	assertRetVal(nSize, false);

	return CreateDataInMemMappedFile(pMem, nSize, nullptr, ppData);
}
#endif // !METRO_APP

bool ff::CreateDataInData(ff::IData* pParentData, size_t nPos, size_t nSize, ff::IData** ppChildData)
{
	assertRetVal(ppChildData, false);
	*ppChildData = nullptr;

	ff::ComPtr<ChildData> pChildData = new ComObject<ChildData>;
	assertRetVal(pChildData->Init(pParentData, nPos, nSize), false);

	*ppChildData = pChildData.Detach();

	return true;
}

bool ff::CreateDataVector(size_t nInitialSize, ff::IDataVector** ppData)
{
	assertRetVal(ppData, false);
	*ppData = nullptr;

	ff::ComPtr<DataVector> pData = new ComObject<DataVector>;
	assertRetVal(pData->Init(nInitialSize), false);

	*ppData = pData.Detach();
	return true;
}

ff::ComPtr<ff::IDataVector> ff::CreateDataVector(ff::Vector<BYTE>&& data)
{
	ff::ComPtr<DataVector> obj = new ff::ComObject<DataVector>;
	obj->Init(std::move(data));
	return obj.Interface();
}

#if METRO_APP
bool ff::CreateDataFromBuffer(Windows::Storage::Streams::IBuffer^ buffer, ff::IData** ppData)
{
	assertRetVal(ppData, false);
	*ppData = nullptr;

	ff::ComPtr<DataBuffer> pData = new ComObject<DataBuffer>;
	assertRetVal(pData->Init(buffer), false);

	*ppData = pData.Detach();
	return true;
}
#endif

Data::Data()
{
}

Data::~Data()
{
	if (_file)
	{
		_file->CloseMemMapped();
	}
}

bool Data::Init(const BYTE* pMem, size_t nSize, ff::IDataFile* pFile)
{
	_file = pFile;
	_mem = pMem;
	_size = nSize;

	if (_file)
	{
		assertRetVal(_file->OpenReadMemMapped(), false);

		if (!_mem)
		{
			assert(_size == ff::INVALID_SIZE);
			_mem = _file->GetMem();
			_size = _file->GetSize();
		}
	}

	return _mem != nullptr;
}

const BYTE* Data::GetMem() const
{
	return _mem;
}

size_t Data::GetSize() const
{
	assertRetVal(_mem, 0);

	return _size;
}

ff::IDataFile* Data::GetFile() const
{
	return _file;
}

bool Data::IsStatic() const
{
	return true;
}

ChildData::ChildData()
{
}

ChildData::~ChildData()
{
}

bool ChildData::Init(ff::IData* pParentData, size_t nPos, size_t nSize)
{
	assertRetVal(
		pParentData &&
		nPos >= 0 &&
		nPos <= pParentData->GetSize() &&
		nSize >= 0 &&
		nPos + nSize <= pParentData->GetSize(),
		false);

	_parentData = pParentData;
	_pos = nPos;
	_size = nSize;

	return true;
}

const BYTE* ChildData::GetMem() const
{
	return _parentData->GetMem() + _pos;
}

size_t ChildData::GetSize() const
{
	return _size;
}

ff::IDataFile* ChildData::GetFile() const
{
	return _parentData->GetFile();
}

bool ChildData::IsStatic() const
{
	return _parentData->IsStatic();
}

DataVector::DataVector()
{
}

DataVector::~DataVector()
{
}

bool DataVector::Init(size_t nInitialSize)
{
	_data.Resize(nInitialSize);

	return _data.Size() == nInitialSize;
}

void DataVector::Init(ff::Vector<BYTE>&& data)
{
	_data = std::move(data);
}

const BYTE* DataVector::GetMem() const
{
	return _data.Data();
}

size_t DataVector::GetSize() const
{
	return _data.Size();
}

ff::IDataFile* DataVector::GetFile() const
{
	return nullptr;
}

bool DataVector::IsStatic() const
{
	return false;
}

ff::Vector<BYTE>& DataVector::GetVector()
{
	return _data;
}

#if METRO_APP

DataBuffer::DataBuffer()
	: _data(nullptr)
{
}

DataBuffer::~DataBuffer()
{
}

bool DataBuffer::Init(Windows::Storage::Streams::IBuffer^ buffer)
{
	assertRetVal(buffer, false);

	ff::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bytes;
	assertRetVal(bytes.QueryFrom(buffer), false);
	assertHrRetVal(bytes->Buffer((byte**)&_data), false);

	_buffer = buffer;
	return true;
}

const BYTE* DataBuffer::GetMem() const
{
	return _data;
}

size_t DataBuffer::GetSize() const
{
	return _buffer->Length;
}

ff::IDataFile* DataBuffer::GetFile() const
{
	return nullptr;
}

bool DataBuffer::IsStatic() const
{
	return false;
}

#endif
