#include "pch.h"
#include "Data/Data.h"
#include "Dict/Dict.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/Texture/Palette.h"
#include "Graph/Texture/PaletteData.h"
#include "Graph/Texture/Texture.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Value/Values.h"
#include "Windows/FileUtil.h"

static ff::StaticString PROP_HASHES(L"hashes");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString PROP_TEXTURE(L"texture");

class __declspec(uuid("d1176b8c-4ebb-40a6-9f6a-f92044187a37"))
	PaletteData
	: public ff::ComBase
	, public ff::IPaletteData
	, public ff::IPalette
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(PaletteData);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(DirectX::ScratchImage&& paletteScratch);

	// IGraphDeviceChild functions
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IPaletteData functions
	virtual size_t GetRowCount() const override;
	virtual ff::hash_t GetRowHash(size_t index) const override;
	virtual ff::ITexture* GetTexture() override;
	virtual ff::ComPtr<ff::IPalette> CreatePalette(float cyclesPerSecond) override;

	// IPalette
	virtual void Advance() override;
	virtual size_t GetCurrentRow() const override;
	virtual ff::IPaletteData* GetData() override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<ff::ITexture> _texture;
	ff::Vector<ff::hash_t> _hashes;
};

BEGIN_INTERFACES(PaletteData)
	HAS_INTERFACE(ff::IPaletteData)
	HAS_INTERFACE(ff::IPalette)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<PaletteData>(L"palette");
	});

bool ff::CreatePaletteData(ff::IGraphDevice* device, DirectX::ScratchImage&& paletteScratch, ff::IPaletteData** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ComPtr<PaletteData, ff::IPaletteData> newObj;
	assertHrRetVal(ff::ComAllocator<PaletteData>::CreateInstance(device, &newObj), false);
	assertRetVal(newObj->Init(std::move(paletteScratch)), false);

	*obj = newObj.Detach();
	return true;
}

PaletteData::PaletteData()
{
}

PaletteData::~PaletteData()
{
	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT PaletteData::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool PaletteData::Init(DirectX::ScratchImage&& paletteScratch)
{
	assertRetVal(paletteScratch.GetImageCount(), false);
	const DirectX::Image& image = *paletteScratch.GetImages();

	_hashes.Reserve(image.height);

	for (const BYTE* start = image.pixels, *cur = start, *end = start + image.height * image.rowPitch; cur < end; cur += image.rowPitch)
	{
		_hashes.Push(ff::HashBytes(cur, ff::PALETTE_ROW_BYTES));
	}

	_texture = _device->AsGraphDeviceInternal()->CreateTexture(std::move(paletteScratch), nullptr);
	return true;
}

ff::IGraphDevice* PaletteData::GetDevice() const
{
	return _device;
}

bool PaletteData::Reset()
{
	return true;
}

size_t PaletteData::GetRowCount() const
{
	return _hashes.Size();
}

ff::hash_t PaletteData::GetRowHash(size_t index) const
{
	return _hashes[index];
}

ff::ITexture* PaletteData::GetTexture()
{
	return _texture;
}

// From Palette.cpp:
bool CreatePalette(ff::IPaletteData* data, float cyclesPerSecond, ff::IPalette** obj);

ff::ComPtr<ff::IPalette> PaletteData::CreatePalette(float cyclesPerSecond)
{
	if (cyclesPerSecond <= 0.0f)
	{
		return this;
	}

	ff::ComPtr<ff::IPalette> palette;
	assertRetVal(::CreatePalette(this, cyclesPerSecond, &palette), nullptr);
	return palette;
}

void PaletteData::Advance()
{
}

size_t PaletteData::GetCurrentRow() const
{
	return 0;
}

ff::IPaletteData* PaletteData::GetData()
{
	return this;
}

bool PaletteData::LoadFromSource(const ff::Dict& dict)
{
	ff::String path = dict.Get<ff::StringValue>(PROP_FILE);
	DirectX::ScratchImage scratch = ff::LoadTextureData(path, DXGI_FORMAT_R8G8B8A8_UNORM, 1, nullptr);
	assertRetVal(scratch.GetImageCount(), false);

	return Init(std::move(scratch));
}

bool PaletteData::LoadFromCache(const ff::Dict& dict)
{
	_texture.QueryFrom(dict.Get<ff::ObjectValue>(::PROP_TEXTURE));
	assertRetVal(_texture, false);

	ff::ComPtr<ff::IData> hashData = dict.Get<ff::DataValue>(::PROP_HASHES);
	assertRetVal(hashData, false);

	_hashes.Resize(hashData->GetSize() / sizeof(ff::hash_t));
	std::memcpy(_hashes.Data(), hashData->GetMem(), _hashes.ByteSize());

	return true;
}

bool PaletteData::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::ObjectValue>(::PROP_TEXTURE, _texture);

	ff::Vector<BYTE> hashes;
	hashes.Resize(_hashes.ByteSize());
	std::memcpy(hashes.Data(), _hashes.Data(), _hashes.ByteSize());

	ff::ComPtr<ff::IDataVector> hashData = ff::CreateDataVector(std::move(hashes));
	dict.Set<ff::DataValue>(::PROP_HASHES, hashData);

	return true;
}
