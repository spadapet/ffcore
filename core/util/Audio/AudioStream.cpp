#include "pch.h"
#include "Audio/AudioStream.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"
#include "Value/Values.h"
#include "Windows/FileUtil.h"

static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString PROP_MIME(L"mime");
static ff::StaticString RES_COMPRESS(L"res:compress");

static ff::StaticString s_defaultMimeType(L"audio/mpeg");

class __declspec(uuid("3e93b56b-a835-4fc7-834e-83e0717ce24c"))
	AudioStream
	: public ff::ComBase
	, public ff::IAudioStream
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(AudioStream);

	// IAudioStream functions
	virtual bool CreateReader(ff::IDataReader** obj) override;
	virtual ff::StringRef GetMimeType() const override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	ff::ComPtr<ff::ISavedData> _data;
	ff::String _mimeType;
};

BEGIN_INTERFACES(AudioStream)
	HAS_INTERFACE(ff::IAudioStream)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup RegisterAudioStream([](ff::Module& module)
	{
		module.RegisterClassT<AudioStream>(L"mp3");
	});

AudioStream::AudioStream()
{
}

AudioStream::~AudioStream()
{
}

bool AudioStream::CreateReader(ff::IDataReader** obj)
{
	assertRetVal(obj && _data, false);
	return _data->CreateSavedDataReader(obj);
}

ff::StringRef AudioStream::GetMimeType() const
{
	return _mimeType;
}

bool AudioStream::LoadFromSource(const ff::Dict& dict)
{
	_mimeType = dict.Get<ff::StringValue>(PROP_MIME, s_defaultMimeType);

	ff::ComPtr<ff::IDataFile> file;
	ff::String fullFile = dict.Get<ff::StringValue>(PROP_FILE);
	assertRetVal(ff::CreateDataFile(fullFile, false, &file), false);
	assertRetVal(ff::CreateSavedDataFromFile(file, 0, file->GetSize(), file->GetSize(), false, &_data), false);

	return true;
}

bool AudioStream::LoadFromCache(const ff::Dict& dict)
{
	_mimeType = dict.Get<ff::StringValue>(PROP_MIME, s_defaultMimeType);
	_data = dict.Get<ff::SavedDataValue>(PROP_DATA);
	assertRetVal(_data, false);

	return true;
}

bool AudioStream::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::StringValue>(PROP_MIME, _mimeType);
	dict.Set<ff::SavedDataValue>(PROP_DATA, _data);
	dict.Set<ff::BoolValue>(RES_COMPRESS, false);

	return true;
}
