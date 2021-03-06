#include "pch.h"
#include "Data/DataWriterReader.h"
#include "Data/Stream.h"
#include "Graph/Font/FontData.h"
#include "UI/Internal/XamlStream.h"
#include "Value/Values.h"

ff::XamlStream::XamlStream(ff::AutoResourceValue&& resource)
	: _resource(std::move(resource))
{
	assert(_resource.DidInit());
}

ff::XamlStream::XamlStream(ff::IDataReader* reader)
	: _reader(reader)
{
	assert(_reader);
}

ff::XamlStream::~XamlStream()
{
}

ff::IDataReader* ff::XamlStream::GetReader() const
{
	if (!_reader && _resource.DidInit())
	{
		XamlStream* self = const_cast<XamlStream*>(this);
		ff::ValuePtr value = self->_resource.Flush();
		ff::ComPtr<IUnknown> comValue = value->GetValue<ff::ObjectValue>();
		ff::ComPtr<ff::IFileStream> file;
		ff::ComPtr<ff::IFontData> fontData;
		ff::ComPtr<ff::IDataReader> reader;

		if (file.QueryFrom(comValue) && file->CreateReader(&reader))
		{
			self->_reader = reader;
		}
		else if (fontData.QueryFrom(comValue) && ff::CreateDataReader(fontData->GetData(), 0, &reader))
		{
			self->_reader = reader;
		}
	}

	assert(_reader);
	return _reader;
}

void ff::XamlStream::SetPosition(uint32_t pos)
{
	ff::IDataReader* reader = GetReader();
	assertRet(reader);

	reader->SetPos(pos);
}

uint32_t ff::XamlStream::GetPosition() const
{
	ff::IDataReader* reader = GetReader();
	assertRetVal(reader, 0);

	return static_cast<uint32_t>(reader->GetPos());
}

uint32_t ff::XamlStream::GetLength() const
{
	ff::IDataReader* reader = GetReader();
	assertRetVal(reader, 0);

	return static_cast<uint32_t>(reader->GetSize());
}

uint32_t ff::XamlStream::Read(void* buffer, uint32_t size)
{
	ff::IDataReader* reader = GetReader();
	assertRetVal(reader, 0);

	size = std::min<uint32_t>(size, GetLength() - GetPosition());

	if (buffer)
	{
		const void* data = reader->Read(size);
		assertRetVal(data, 0);
		std::memcpy(buffer, data, size);
	}
	else
	{
		SetPosition(GetPosition() + size);
	}

	return size;
}

void ff::XamlStream::Close()
{
	_resource = ff::AutoResourceValue();
	_reader = nullptr;
}
