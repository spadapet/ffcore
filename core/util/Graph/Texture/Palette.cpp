#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Graph/GraphDevice.h"
#include "Graph/Texture/Palette.h"
#include "Graph/Texture/PaletteData.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"

class __declspec(uuid("f30b7600-f6e2-47ca-a323-30af3186d3dd"))
	Palette
	: public ff::ComBase
	, public ff::IPalette
{
public:
	DECLARE_HEADER(Palette);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(ff::IPaletteData* data);

	// IGraphDeviceChild functions
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IPalette functions
	virtual void Advance() override;
	virtual ff::IPaletteData* GetData() const override;
	virtual ff::hash_t GetTextureHash() const override;
	virtual ff::ITexture* GetTexture() override;

private:
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<ff::IPaletteData> _data;
	ff::ComPtr<ff::ITexture> _texture;
	std::array<DWORD, ff::PALETTE_SIZE> _colors;
	ff::hash_t _textureHash;
	ff::hash_t _colorsHash;
};

BEGIN_INTERFACES(Palette)
	HAS_INTERFACE(ff::IPalette)
END_INTERFACES()

bool ff::CreatePalette(IGraphDevice* device, IPaletteData* data, IPalette** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ComPtr<Palette, IPalette> newObj;
	assertHrRetVal(ff::ComAllocator<Palette>::CreateInstance(device, &newObj), false);
	assertRetVal(newObj->Init(data), false);

	*obj = newObj.Detach();
	return true;
}

Palette::Palette()
	: _textureHash(0)
	, _colorsHash(0)
{
}

Palette::~Palette()
{
	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT Palette::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool Palette::Init(ff::IPaletteData* data)
{
	assertRetVal(data && data->GetColors() && data->GetColors()->GetSize() == _colors.size() * 4, false);

	_data = data;
	_colorsHash = ff::HashBytes(_data->GetColors()->GetMem(), _data->GetColors()->GetSize());
	std::memcpy(_colors.data(), _data->GetColors()->GetMem(), _data->GetColors()->GetSize());

	return true;
}

ff::IGraphDevice* Palette::GetDevice() const
{
	return _device;
}

bool Palette::Reset()
{
	_texture = nullptr;
	_textureHash = 0;

	return true;
}

void Palette::Advance()
{
	// ...update _colors and _colorsHash
}

ff::IPaletteData* Palette::GetData() const
{
	return _data;
}

ff::hash_t Palette::GetTextureHash() const
{
	return _colorsHash;
}

ff::ITexture* Palette::GetTexture()
{
	if (_texture == nullptr)
	{
		_texture = _device->CreateTexture(ff::PointInt((int)ff::PALETTE_SIZE, 1), ff::TextureFormat::RGBA32, 1, 1, 1, _data->GetColors());
		_textureHash = ff::HashBytes(_data->GetColors()->GetMem(), _data->GetColors()->GetSize());
	}

	if (_textureHash != _colorsHash)
	{
		_texture->Update(0, 0, ff::RectSize(0, 0, ff::PALETTE_SIZE, 1), _colors.data(), ff::PALETTE_SIZE * 4, ff::TextureFormat::RGBA32);
		_textureHash = _colorsHash;
	}

	return _texture;
}
