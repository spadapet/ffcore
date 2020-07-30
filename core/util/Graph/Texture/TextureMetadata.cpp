#include "pch.h"
#include "Dict/Dict.h"
#include "Graph/Texture/Texture.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Value/Values.h"

static ff::StaticString PROP_ARRAY_SIZE(L"arraySize");
static ff::StaticString PROP_FORMAT(L"format");
static ff::StaticString PROP_MIPS(L"mips");
static ff::StaticString PROP_SIZE(L"size");

class __declspec(uuid("a88df538-e3e4-4308-8112-49c0c2c80682"))
	TextureMetadata
	: public ff::ComBase
	, public ff::ITextureMetadata
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(TextureMetadata);

	bool Init(ff::ITexture* texture);

	// ITextureMetadata
	virtual ff::PointInt GetSize() const override;
	virtual size_t GetMipCount() const override;
	virtual size_t GetArraySize() const override;
	virtual ff::TextureFormat GetFormat() const override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	ff::PointInt _size;
	size_t _mips;
	size_t _arraySize;
	ff::TextureFormat _format;
};

BEGIN_INTERFACES(TextureMetadata)
	HAS_INTERFACE(ff::ITextureMetadata)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup RegisterTexture([](ff::Module& module)
	{
		module.RegisterClassT<TextureMetadata>(L"textureMetadata");
	});

ff::ComPtr<ff::ITextureMetadata> CreateTextureMetadata(ff::ITexture* texture)
{
	ff::ComPtr<TextureMetadata, ff::ITextureMetadata> myObj;
	assertHrRetVal(ff::ComAllocator<TextureMetadata>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(texture), false);

	return ff::ComPtr<ff::ITextureMetadata>(myObj);
}

TextureMetadata::TextureMetadata()
	: _size(0, 0)
	, _mips(0)
	, _arraySize(0)
	, _format(ff::TextureFormat::Unknown)
{
}

TextureMetadata::~TextureMetadata()
{
}

bool TextureMetadata::Init(ff::ITexture* texture)
{
	assertRetVal(texture, false);

	_size = texture->GetSize();
	_mips = texture->GetMipCount();
	_arraySize = texture->GetArraySize();
	_format = texture->GetFormat();

	return true;
}

ff::PointInt TextureMetadata::GetSize() const
{
	return _size;
}

size_t TextureMetadata::GetMipCount() const
{
	return _mips;
}

size_t TextureMetadata::GetArraySize() const
{
	return _arraySize;
}

ff::TextureFormat TextureMetadata::GetFormat() const
{
	return _format;
}

bool TextureMetadata::LoadFromSource(const ff::Dict& dict)
{
	assertRetVal(false, false);
}

bool TextureMetadata::LoadFromCache(const ff::Dict& dict)
{
	_size = dict.Get<ff::PointIntValue>(::PROP_SIZE);
	_mips = dict.Get<ff::SizeValue>(::PROP_MIPS);
	_arraySize = dict.Get<ff::SizeValue>(::PROP_ARRAY_SIZE);
	_format = (ff::TextureFormat)dict.Get<ff::IntValue>(::PROP_FORMAT);

	return true;
}

bool TextureMetadata::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::PointIntValue>(::PROP_SIZE, _size);
	dict.Set<ff::SizeValue>(::PROP_MIPS, _mips);
	dict.Set<ff::SizeValue>(::PROP_ARRAY_SIZE, _arraySize);
	dict.Set<ff::IntValue>(::PROP_FORMAT, (int)_format);
	dict.Set<ff::BoolValue>(ff::RES_COMPRESS, false);

	return true;
}
