#include "pch.h"
#include "Audio/AudioBuffer.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"
#include "Value/Values.h"

static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString PROP_FORMAT(L"format");

static constexpr DWORD MakeFourCC(char ch0, char ch1, char ch2, char ch3)
{
	return
		(DWORD)ch0 |
		((DWORD)ch1 << 8) |
		((DWORD)ch2 << 16) |
		((DWORD)ch3 << 24);
}

class RiffFileHelper
{
public:
	bool Read(ff::IDataReader* reader);

protected:
	virtual bool HandleData(DWORD id, ff::ISavedData* savedData) = 0;

private:
	bool InternalRead(ff::IDataReader* reader);
};

bool RiffFileHelper::Read(ff::IDataReader* reader)
{
	DWORD id;
	assertRetVal(ff::LoadData(reader, id), false);
	assertRetVal(id == ::MakeFourCC('R', 'I', 'F', 'F'), false);

	DWORD size;
	assertRetVal(ff::LoadData(reader, size), false);
	assertRetVal(ff::LoadData(reader, id), false);

	ff::ComPtr<ff::ISavedData> data;
	assertRetVal(reader->CreateSavedData(reader->GetPos(), size, size, false, &data), false);
	assertRetVal(HandleData(id, data), false);

	return InternalRead(reader);
}

bool RiffFileHelper::InternalRead(ff::IDataReader* reader)
{
	while (reader->GetPos() < reader->GetSize())
	{
		DWORD id, size;
		assertRetVal(ff::LoadData(reader, id), false);
		assertRetVal(ff::LoadData(reader, size), false);

		ff::ComPtr<ff::ISavedData> data;
		assertRetVal(reader->CreateSavedData(reader->GetPos(), size, size, false, &data), false);
		assertRetVal(reader->SetPos(reader->GetPos() + size), false);

		HandleData(id, data);
	}

	return true;
}

class WaveFileHelper : public RiffFileHelper
{
public:
	WaveFileHelper();

	WAVEFORMATEX _format;
	ff::ComPtr<ff::IData> _data;

protected:
	virtual bool HandleData(DWORD id, ff::ISavedData* savedData);
};

WaveFileHelper::WaveFileHelper()
{
	ff::ZeroObject(_format);
}

bool WaveFileHelper::HandleData(DWORD id, ff::ISavedData* savedData)
{
	switch (id)
	{
	case ::MakeFourCC('W', 'A', 'V', 'E'):
		return true;

	case ::MakeFourCC('f', 'm', 't', ' '):
		if (savedData->GetFullSize() >= sizeof(PCMWAVEFORMAT))
		{
			std::memcpy(&_format, savedData->Load()->GetMem(), sizeof(PCMWAVEFORMAT));
			return true;
		}
		break;

	case ::MakeFourCC('d', 'a', 't', 'a'):
		_data = savedData->Load();
		return _data != nullptr;
	}

	return false;
}

class __declspec(uuid("383db502-512c-4892-94d2-2cdaeccb754d"))
	AudioBuffer
	: public ff::ComBase
	, public ff::IAudioBuffer
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(AudioBuffer);

	// IAudioBuffer functions
	virtual const WAVEFORMATEX& GetFormat() const override;
	virtual ff::IData* GetData() const override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	WAVEFORMATEX _format;
	ff::ComPtr<ff::IData> _data;
};

BEGIN_INTERFACES(AudioBuffer)
	HAS_INTERFACE(ff::IAudioBuffer)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup RegisterAudioBuffer([](ff::Module& module)
	{
		module.RegisterClassT<AudioBuffer>(L"wave");
	});

AudioBuffer::AudioBuffer()
{
}

AudioBuffer::~AudioBuffer()
{
}

const WAVEFORMATEX& AudioBuffer::GetFormat() const
{
	return _format;
}

ff::IData* AudioBuffer::GetData() const
{
	return _data;
}

bool AudioBuffer::LoadFromSource(const ff::Dict& dict)
{
	ff::ComPtr<ff::IDataFile> file;
	ff::String fullFile = dict.Get<ff::StringValue>(PROP_FILE);
	assertRetVal(ff::CreateDataFile(fullFile, false, &file), false);

	ff::ComPtr<ff::IDataReader> reader;
	assertRetVal(ff::CreateDataReader(file, 0, &reader), false);

	WaveFileHelper waveHelper;
	waveHelper.Read(reader);

	_data = waveHelper._data;
	_format = waveHelper._format;

	assertRetVal(_data && _format.wFormatTag != 0, false);

	return true;
}

bool AudioBuffer::LoadFromCache(const ff::Dict& dict)
{
	_data = dict.Get<ff::DataValue>(PROP_DATA);
	ff::ComPtr<ff::IData> formatData = dict.Get<ff::DataValue>(PROP_FORMAT);
	assertRetVal(_data && formatData && formatData->GetSize() == sizeof(_format), false);
	_format = *(const WAVEFORMATEX*)formatData->GetMem();

	return true;
}

bool AudioBuffer::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::DataValue>(PROP_DATA, _data);
	dict.Set<ff::DataValue>(PROP_FORMAT, &_format, sizeof(_format));

	return true;
}
