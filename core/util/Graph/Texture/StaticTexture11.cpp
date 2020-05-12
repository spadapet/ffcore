#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Dict/Dict.h"
#include "Graph/DataBlob.h"
#include "Graph/DirectXUtil.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphDeviceChild.h"
#include "Graph/Render/RenderAnimation.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteType.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"
#include "Value/Values.h"

static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString PROP_FORMAT(L"format");
static ff::StaticString PROP_MIPS(L"mips");
static ff::StaticString PROP_SPRITE_TYPE(L"spriteType");

class __declspec(uuid("67741046-5d2f-4bf6-a955-baa950401935"))
	StaticTexture11
	: public ff::ComBase
	, public ff::ITexture
	, public ff::ITextureDxgi
	, public ff::ITexture11
	, public ff::ITextureView
	, public ff::ITextureView11
	, public ff::IResourcePersist
	, public ff::ISprite
	, public ff::IRenderAnimation
{
public:
	DECLARE_HEADER(StaticTexture11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(DirectX::ScratchImage&& data);

	// IGraphDeviceChild
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// ITextureView11
	virtual ff::ITexture* GetTexture() override;
	virtual ITextureView11* AsTextureView11() override;
	virtual ID3D11ShaderResourceView* GetView() override;

	// ITexture
	virtual ff::PointInt GetSize() const override;
	virtual size_t GetMipCount() const override;
	virtual size_t GetArraySize() const override;
	virtual size_t GetSampleCount() const override;
	virtual ff::TextureFormat GetFormat() const override;
	virtual ff::SpriteType GetSpriteType() const override;
	virtual ff::ComPtr<ff::ITextureView> CreateView(size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount) override;
	virtual ff::ComPtr<ff::ITexture> Convert(ff::TextureFormat format, size_t mips) override;
	virtual ff::ISprite* AsSprite() override;
	virtual ff::ITextureView* AsTextureView() override;
	virtual ff::ITextureDxgi* AsTextureDxgi() override;
	virtual ff::ITexture11* AsTexture11() override;

	// ITextureDxgi
	virtual DXGI_FORMAT GetDxgiFormat() const override;
	virtual const DirectX::ScratchImage* Capture(DirectX::ScratchImage& tempHolder) override;

	// ITexture11
	virtual ID3D11Texture2D* GetTexture2d() override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

	// ISprite
	virtual const ff::SpriteData& GetSpriteData() override;

	// IRenderAnimation functions
	virtual void Render(
		ff::IRendererActive* render,
		ff::AnimTweenType type,
		float frame,
		ff::PointFloat pos,
		ff::PointFloat scale,
		float rotate,
		const DirectX::XMFLOAT4& color) override;
	virtual float GetLastFrame() const override;
	virtual float GetFPS() const override;

private:
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<ID3D11Texture2D> _texture;
	ff::ComPtr<ID3D11ShaderResourceView> _view;
	std::unique_ptr<ff::SpriteData> _spriteData;
	DirectX::ScratchImage _originalData;
	ff::SpriteType _spriteType;
};

BEGIN_INTERFACES(StaticTexture11)
	HAS_INTERFACE(ff::ITexture)
	HAS_INTERFACE(ff::ITextureView)
	HAS_INTERFACE(ff::IResourcePersist)
	HAS_INTERFACE(ff::ISprite)
	HAS_INTERFACE(ff::IRenderAnimation)
END_INTERFACES()

static ff::ModuleStartup RegisterTexture([](ff::Module& module)
	{
		module.RegisterClassT<StaticTexture11>(L"texture");
	});

bool CreateTexture11(ff::IGraphDevice* device, DirectX::ScratchImage&& data, ff::ITexture** texture)
{
	assertRetVal(texture, false);

	ff::ComPtr<StaticTexture11, ff::ITexture> obj;
	assertHrRetVal(ff::ComAllocator<StaticTexture11>::CreateInstance(device, &obj), false);
	assertRetVal(obj->Init(std::move(data)), false);

	*texture = obj.Detach();
	return true;
}

bool CreateTexture11(ff::IGraphDevice* device, ff::StringRef path, DXGI_FORMAT format, size_t mips, ff::ITexture** texture)
{
	assertRetVal(texture, false);

	DirectX::ScratchImage data = ff::LoadTextureData(device, path, format, mips);
	assertRetVal(data.GetImageCount(), false);

	ff::ComPtr<ff::ITexture> obj;
	assertRetVal(::CreateTexture11(device, std::move(data), &obj), false);

	*texture = obj.Detach();
	return true;
}

StaticTexture11::StaticTexture11()
	: _spriteType(ff::SpriteType::Unknown)
{
}

StaticTexture11::~StaticTexture11()
{
	if (_device)
	{
		_device->RemoveChild(static_cast<ff::ITexture*>(this));
	}
}

HRESULT StaticTexture11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(static_cast<ff::ITexture*>(this));

	return __super::_Construct(unkOuter);
}

bool StaticTexture11::Init(DirectX::ScratchImage&& data)
{
	assertRetVal(data.GetImageCount(), false);
	_originalData = std::move(data);
	_spriteType = ff::GetSpriteTypeForImage(_originalData);

	return true;
}

ff::IGraphDevice* StaticTexture11::GetDevice() const
{
	return _device;
}

bool StaticTexture11::Reset()
{
	_texture = nullptr;
	_view = nullptr;
	return true;
}

ff::PointInt StaticTexture11::GetSize() const
{
	return ff::PointInt(
		(int)_originalData.GetMetadata().width,
		(int)_originalData.GetMetadata().height);
}

size_t StaticTexture11::GetMipCount() const
{
	return _originalData.GetMetadata().mipLevels;
}

size_t StaticTexture11::GetArraySize() const
{
	return _originalData.GetMetadata().arraySize;
}

size_t StaticTexture11::GetSampleCount() const
{
	return 1;
}

ff::TextureFormat StaticTexture11::GetFormat() const
{
	return ff::ConvertTextureFormat(GetDxgiFormat());
}

ff::SpriteType StaticTexture11::GetSpriteType() const
{
	return _spriteType;
}

bool CreateTextureView11(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount, ff::ITextureView** obj);

ff::ComPtr<ff::ITextureView> StaticTexture11::CreateView(size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount)
{
	ff::ComPtr<ff::ITextureView> obj;
	return ::CreateTextureView11(this, arrayStart, arrayCount, mipStart, mipCount, &obj) ? obj : nullptr;
}

ff::ISprite* StaticTexture11::AsSprite()
{
	return this;
}

ff::ITextureView* StaticTexture11::AsTextureView()
{
	return this;
}

ff::ITextureDxgi* StaticTexture11::AsTextureDxgi()
{
	return this;
}

ff::ITexture11* StaticTexture11::AsTexture11()
{
	return this;
}

DXGI_FORMAT StaticTexture11::GetDxgiFormat() const
{
	return _originalData.GetMetadata().format;
}

ff::ITexture* StaticTexture11::GetTexture()
{
	return this;
}

ID3D11Texture2D* StaticTexture11::GetTexture2d()
{
	if (!_texture)
	{
		ff::ComPtr<ID3D11Resource> resource;
		assertHrRetVal(DirectX::CreateTexture(
			_device->AsGraphDevice11()->Get3d(),
			_originalData.GetImages(),
			_originalData.GetImageCount(),
			_originalData.GetMetadata(),
			&resource), nullptr);

		ff::ComPtr<ID3D11Texture2D> texture;
		assertRetVal(_texture.QueryFrom(resource), nullptr);
	}

	return _texture;
}

ff::ITextureView11* StaticTexture11::AsTextureView11()
{
	return this;
}

ID3D11ShaderResourceView* StaticTexture11::GetView()
{
	if (!_view)
	{
		_view = ff::CreateDefaultTextureView(_device->AsGraphDevice11()->Get3d(), GetTexture2d());
	}

	return _view;
}

ff::ComPtr<ff::ITexture> StaticTexture11::Convert(ff::TextureFormat format, size_t mips)
{
	DXGI_FORMAT dxgiFormat = ff::ConvertTextureFormat(format);
	if (_originalData.GetMetadata().format == dxgiFormat && mips && _originalData.GetMetadata().mipLevels >= mips)
	{
		return this;
	}

	DirectX::ScratchImage data = ff::ConvertTextureData(_originalData, dxgiFormat, mips);
	return _device->AsGraphDeviceInternal()->CreateTexture(std::move(data));
}

const DirectX::ScratchImage* StaticTexture11::Capture(DirectX::ScratchImage& tempHolder)
{
	return &_originalData;
}

bool StaticTexture11::LoadFromSource(const ff::Dict& dict)
{
	size_t mipsProp = dict.Get<ff::SizeValue>(PROP_MIPS, 1);
	ff::String fullFile = dict.Get<ff::StringValue>(PROP_FILE);
	DXGI_FORMAT format = ff::ParseDxgiTextureFormat(dict.Get<ff::StringValue>(PROP_FORMAT, ff::String(L"rgba32")));
	assertRetVal(format != DXGI_FORMAT_UNKNOWN, false);

	DirectX::ScratchImage data = ff::LoadTextureData(_device, fullFile, format, mipsProp);
	assertRetVal(Init(std::move(data)), false);

	return true;
}

bool StaticTexture11::LoadFromCache(const ff::Dict& dict)
{
	ff::ComPtr<ff::IData> data = dict.Get<ff::DataValue>(PROP_DATA);
	_spriteType = (ff::SpriteType)dict.Get<ff::IntValue>(PROP_SPRITE_TYPE);

	assertHrRetVal(DirectX::LoadFromDDSMemory(
		data->GetMem(),
		data->GetSize(),
		DirectX::DDS_FLAGS_NONE,
		nullptr,
		_originalData), false);

	return true;
}

bool StaticTexture11::SaveToCache(ff::Dict& dict)
{
	DirectX::Blob blob;
	assertHrRetVal(DirectX::SaveToDDSMemory(
		_originalData.GetImages(),
		_originalData.GetImageCount(),
		_originalData.GetMetadata(),
		DirectX::DDS_FLAGS_NONE,
		blob), false);

	ff::ComPtr<ff::IData> blobData;
	assertRetVal(ff::CreateDataFromBlob(std::move(blob), &blobData), false);

	dict.Set<ff::DataValue>(PROP_DATA, blobData);
	dict.Set<ff::IntValue>(PROP_SPRITE_TYPE, (int)_spriteType);

	return true;
}

const ff::SpriteData& StaticTexture11::GetSpriteData()
{
	if (!_spriteData)
	{
		_spriteData = std::make_unique<ff::SpriteData>();
		_spriteData->_textureView = this;
		_spriteData->_textureUV.SetRect(0, 0, 1, 1);
		_spriteData->_worldRect = ff::RectFloat(GetSize().ToType<float>());
		_spriteData->_type = _spriteType;
	}

	return *_spriteData;
}

void StaticTexture11::Render(
	ff::IRendererActive* render,
	ff::AnimTweenType type,
	float frame,
	ff::PointFloat pos,
	ff::PointFloat scale,
	float rotate,
	const DirectX::XMFLOAT4& color)
{
	render->DrawSprite(this, pos, scale, rotate, color);
}

float StaticTexture11::GetLastFrame() const
{
	return 0;
}

float StaticTexture11::GetFPS() const
{
	return 0;
}
