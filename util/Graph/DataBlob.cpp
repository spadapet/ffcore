#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Data.h"
#include "Graph/DataBlob.h"

class __declspec(uuid("ea478997-3767-4438-8234-a3b2ab31ee89"))
	DataBlob : public ff::ComBase, public ff::IData
{
public:
	DECLARE_HEADER(DataBlob);

	bool Init(ID3DBlob* blob);

	// IData functions
	virtual const BYTE* GetMem() const override;
	virtual size_t GetSize() const override;
	virtual ff::IDataFile* GetFile() const override;
	virtual bool IsStatic() const override;

private:
	ff::ComPtr<ID3DBlob> _blob;
};

BEGIN_INTERFACES(DataBlob)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

class __declspec(uuid("8cd06f91-e7d2-4b3f-8624-33ccde129b5e"))
	DataTexBlob : public ff::ComBase, public ff::IData
{
public:
	DECLARE_HEADER(DataTexBlob);

	bool Init(DirectX::Blob&& blob);

	// IData functions
	virtual const BYTE* GetMem() const override;
	virtual size_t GetSize() const override;
	virtual ff::IDataFile* GetFile() const override;
	virtual bool IsStatic() const override;

private:
	DirectX::Blob _blob;
};

BEGIN_INTERFACES(DataTexBlob)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

bool ff::CreateDataFromBlob(ID3DBlob* blob, ff::IData** obj)
{
	assertRetVal(blob && obj, false);
	*obj = nullptr;

	ff::ComPtr<DataBlob> myObj = new ComObject<DataBlob>;
	assertRetVal(myObj->Init(blob), false);

	*obj = myObj.Detach();
	return *obj != nullptr;
}

bool ff::CreateDataFromBlob(DirectX::Blob&& blob, ff::IData** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ComPtr<DataTexBlob> myObj = new ComObject<DataTexBlob>;
	assertRetVal(myObj->Init(std::move(blob)), false);

	*obj = myObj.Detach();
	return *obj != nullptr;
}

DataBlob::DataBlob()
{
}

DataBlob::~DataBlob()
{
}

bool DataBlob::Init(ID3DBlob* blob)
{
	assertRetVal(blob, false);

	_blob = blob;

	return true;
}

const BYTE* DataBlob::GetMem() const
{
	return _blob ? (const BYTE*)_blob->GetBufferPointer() : 0;
}

size_t DataBlob::GetSize() const
{
	return _blob ? _blob->GetBufferSize() : 0;
}

ff::IDataFile* DataBlob::GetFile() const
{
	return nullptr;
}

bool DataBlob::IsStatic() const
{
	return false;
}

DataTexBlob::DataTexBlob()
{
}

DataTexBlob::~DataTexBlob()
{
}

bool DataTexBlob::Init(DirectX::Blob&& blob)
{
	assertRetVal(blob.GetBufferPointer(), false);
	_blob = std::move(blob);
	return true;
}

const BYTE* DataTexBlob::GetMem() const
{
	return (const BYTE*)_blob.GetBufferPointer();
}

size_t DataTexBlob::GetSize() const
{
	return _blob.GetBufferSize();
}

ff::IDataFile* DataTexBlob::GetFile() const
{
	return nullptr;
}

bool DataTexBlob::IsStatic() const
{
	return false;
}
