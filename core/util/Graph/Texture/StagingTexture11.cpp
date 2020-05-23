#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Dict/Dict.h"
#include "Graph/DataBlob.h"
#include "Graph/GraphDevice.h"
#include "Graph/Render/RenderAnimation.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteType.h"
#include "Graph/State/GraphContext11.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "Resource/ResourcePersist.h"
#include "Thread/ThreadDispatch.h"

static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_SPRITE_TYPE(L"spriteType");

class __declspec(uuid("94ba864d-9773-4ddd-8ede-bc39335e6852"))
	StagingTexture11
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
	DECLARE_HEADER(StagingTexture11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(const D3D11_TEXTURE2D_DESC& textureDesc, ff::IData* initialData);

	// IGraphDeviceChild
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// ITextureView11
	virtual ff::ITexture* GetTexture() override;
	virtual ff::ITextureView11* AsTextureView11() override;
	virtual ID3D11ShaderResourceView* GetView() override;

	// ITexture
	virtual ff::PointInt GetSize() const override;
	virtual size_t GetMipCount() const override;
	virtual size_t GetArraySize() const override;
	virtual size_t GetSampleCount() const override;
	virtual ff::TextureFormat GetFormat() const override;
	virtual ff::SpriteType GetSpriteType() const override;
	virtual ff::IPalette* GetPalette() const override;
	virtual ff::ComPtr<ff::ITextureView> CreateView(size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount) override;
	virtual ff::ComPtr<ff::ITexture> Convert(ff::TextureFormat format, size_t mips) override;
	virtual void Update(size_t arrayIndex, size_t mipIndex, const ff::RectSize& rect, const void* data, size_t rowPitch, ff::TextureFormat dataFormat) override;
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
	ff::ComPtr<ff::IData> _initialData;
	std::unique_ptr<ff::SpriteData> _spriteData;
	D3D11_TEXTURE2D_DESC _textureDesc;
};

BEGIN_INTERFACES(StagingTexture11)
	HAS_INTERFACE(ff::ITexture)
	HAS_INTERFACE(ff::ITextureView)
	HAS_INTERFACE(ff::IResourcePersist)
	HAS_INTERFACE(ff::ISprite)
	HAS_INTERFACE(ff::IRenderAnimation)
END_INTERFACES()

bool CreateTexture11(
	ff::IGraphDevice* device,
	ff::PointInt size,
	DXGI_FORMAT format,
	size_t mips,
	size_t count,
	size_t samples,
	ff::IData* initialData,
	ff::ITexture** texture)
{
	assertRetVal(device && texture && size.x > 0 && size.y > 0, false);
	assertRetVal(mips > 0 && count > 0 && samples > 0 && ff::NearestPowerOfTwo(samples) == samples, false);

	// compressed textures must have sizes that are multiples of four
	if (DirectX::IsCompressed(format) && (size.x % 4 || size.y % 4))
	{
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	while (samples > 1)
	{
		UINT levels;
		if (FAILED(device->AsGraphDevice11()->Get3d()->CheckMultisampleQualityLevels(format, (UINT)samples, &levels)) || !levels)
		{
			samples /= 2;
		}
	}

	D3D11_TEXTURE2D_DESC desc;
	ff::ZeroObject(desc);

	desc.Width = size.x;
	desc.Height = size.y;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (!DirectX::IsCompressed(format) ? D3D11_BIND_RENDER_TARGET : 0);
	desc.Format = format;
	desc.MipLevels = (UINT)mips;
	desc.ArraySize = (UINT)count;
	desc.SampleDesc.Count = (UINT)samples;

	ff::ComPtr<StagingTexture11, ff::ITexture> obj;
	assertHrRetVal(ff::ComAllocator<StagingTexture11>::CreateInstance(device, &obj), false);
	assertRetVal(obj->Init(desc, initialData), false);

	*texture = obj.Detach();
	return true;
}

bool CreateStagingTexture11(
	ff::IGraphDevice* device,
	ff::PointInt size,
	DXGI_FORMAT format,
	bool readable,
	bool writable,
	size_t mips,
	size_t count,
	size_t samples,
	ff::IData* initialData,
	ff::ITexture** texture)
{
	assertRetVal(device && texture && size.x > 0 && size.y > 0, false);
	assertRetVal(mips > 0 && count > 0 && samples > 0 && ff::NearestPowerOfTwo(samples) == samples, false);
	assertRetVal(readable || writable, false);
	assertRetVal(!DirectX::IsCompressed(format), false);

	while (samples > 1)
	{
		UINT levels;
		if (FAILED(device->AsGraphDevice11()->Get3d()->CheckMultisampleQualityLevels(format, (UINT)samples, &levels)) || !levels)
		{
			samples /= 2;
		}
	}

	D3D11_TEXTURE2D_DESC desc;
	ff::ZeroObject(desc);

	desc.Width = size.x;
	desc.Height = size.y;
	desc.Usage = readable ? D3D11_USAGE_STAGING : D3D11_USAGE_DYNAMIC;
	desc.BindFlags = readable ? 0 : D3D11_BIND_SHADER_RESOURCE;
	desc.Format = format;
	desc.CPUAccessFlags = (writable ? D3D11_CPU_ACCESS_WRITE : 0) | (readable ? D3D11_CPU_ACCESS_READ : 0);
	desc.MipLevels = (UINT)mips;
	desc.ArraySize = (UINT)count;
	desc.SampleDesc.Count = (UINT)samples;

	ff::ComPtr<StagingTexture11, ff::ITexture> obj;
	assertHrRetVal(ff::ComAllocator<StagingTexture11>::CreateInstance(device, &obj), false);
	assertRetVal(obj->Init(desc, initialData), false);

	*texture = obj.Detach();
	return true;
}

StagingTexture11::StagingTexture11()
{
	ff::ZeroObject(_textureDesc);
}

StagingTexture11::~StagingTexture11()
{
	if (_device)
	{
		_device->RemoveChild(static_cast<ff::ITexture*>(this));
	}
}

HRESULT StagingTexture11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(static_cast<ff::ITexture*>(this));

	return __super::_Construct(unkOuter);
}

bool StagingTexture11::Init(const D3D11_TEXTURE2D_DESC& textureDesc, ff::IData* initialData)
{
	_textureDesc = textureDesc;
	_initialData = initialData;
	return true;
}

ff::IGraphDevice* StagingTexture11::GetDevice() const
{
	return _device;
}

bool StagingTexture11::Reset()
{
	_texture = nullptr;
	_view = nullptr;
	return true;
}

ff::PointInt StagingTexture11::GetSize() const
{
	return ff::PointInt(_textureDesc.Width, _textureDesc.Height);
}

size_t StagingTexture11::GetMipCount() const
{
	return _textureDesc.MipLevels;
}

size_t StagingTexture11::GetArraySize() const
{
	return _textureDesc.ArraySize;
}

size_t StagingTexture11::GetSampleCount() const
{
	return _textureDesc.SampleDesc.Count;
}

ff::TextureFormat StagingTexture11::GetFormat() const
{
	return ff::ConvertTextureFormat(GetDxgiFormat());
}

ff::SpriteType StagingTexture11::GetSpriteType() const
{
	return (GetDxgiFormat() == DXGI_FORMAT_R8_UINT)
		? ff::SpriteType::OpaquePalette
		: (DirectX::HasAlpha(GetDxgiFormat()) ? ff::SpriteType::Transparent : ff::SpriteType::Opaque);
}

ff::IPalette* StagingTexture11::GetPalette() const
{
	return nullptr;
}

bool CreateTextureView11(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount, ff::ITextureView** obj);

ff::ComPtr<ff::ITextureView> StagingTexture11::CreateView(size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount)
{
	ff::ComPtr<ff::ITextureView> obj;
	return ::CreateTextureView11(this, arrayStart, arrayCount, mipStart, mipCount, &obj) ? obj : nullptr;
}

ff::ISprite* StagingTexture11::AsSprite()
{
	return this;
}

ff::ITextureView* StagingTexture11::AsTextureView()
{
	return this;
}

ff::ITextureDxgi* StagingTexture11::AsTextureDxgi()
{
	return this;
}

ff::ITexture11* StagingTexture11::AsTexture11()
{
	return this;
}

DXGI_FORMAT StagingTexture11::GetDxgiFormat() const
{
	return _textureDesc.Format;
}

ff::ITexture* StagingTexture11::GetTexture()
{
	return this;
}

ID3D11Texture2D* StagingTexture11::GetTexture2d()
{
	if (!_texture)
	{
		D3D11_SUBRESOURCE_DATA data{ 0 };
		const D3D11_SUBRESOURCE_DATA* initialData = nullptr;
		size_t rowPitch, slicePitch;

		if (_initialData && SUCCEEDED(DirectX::ComputePitch(_textureDesc.Format, _textureDesc.Width, _textureDesc.Height, rowPitch, slicePitch)))
		{
			if (_initialData->GetSize() == rowPitch * _textureDesc.Height)
			{
				data.pSysMem = _initialData->GetMem();
				data.SysMemPitch = (UINT)rowPitch;
				data.SysMemSlicePitch = (UINT)slicePitch;
				initialData = &data;
			}
			else
			{
				assertSz(false, L"Invalid initial data for scratch texture");
			}
		}

		assertHrRetVal(_device->AsGraphDevice11()->Get3d()->CreateTexture2D(&_textureDesc, initialData, &_texture), nullptr);
	}

	return _texture;
}

ff::ITextureView11* StagingTexture11::AsTextureView11()
{
	return this;
}

ID3D11ShaderResourceView* StagingTexture11::GetView()
{
	if (!_view)
	{
		_view = ff::CreateDefaultTextureView(_device->AsGraphDevice11()->Get3d(), GetTexture2d());
	}

	return _view;
}

ff::ComPtr<ff::ITexture> StagingTexture11::Convert(ff::TextureFormat format, size_t mips)
{
	assertRetVal(GetTexture2d(), false);

	DirectX::ScratchImage scratch;
	assertRetVal(Capture(scratch) == &scratch, false);

	DirectX::ScratchImage data = ff::ConvertTextureData(scratch, ff::ConvertTextureFormat(format), mips);
	return _device->AsGraphDeviceInternal()->CreateTexture(std::move(data), nullptr);
}

void StagingTexture11::Update(size_t arrayIndex, size_t mipIndex, const ff::RectSize& rect, const void* data, size_t rowPitch, ff::TextureFormat dataFormat)
{
	assertRet(GetFormat() == dataFormat);

	CD3D11_BOX box((UINT)rect.left, (UINT)rect.top, 0, (UINT)rect.right, (UINT)rect.bottom, 1);
	UINT subResource = ::D3D11CalcSubresource((UINT)mipIndex, (UINT)arrayIndex, (UINT)_textureDesc.MipLevels);
	_device->AsGraphDevice11()->GetStateContext().UpdateSubresource(GetTexture2d(), subResource, &box, data, (UINT)rowPitch, 0);
}

const DirectX::ScratchImage* StagingTexture11::Capture(DirectX::ScratchImage& tempHolder)
{
	ff::ComPtr<StagingTexture11, ff::ITexture> keepAlive = this;
	HRESULT success = E_FAIL;

	// Can't use the device context on background threads
	ff::GetGameThreadDispatch()->Send([keepAlive, &tempHolder, &success]()
		{
			success = DirectX::CaptureTexture(
				keepAlive->_device->AsGraphDevice11()->Get3d(),
				keepAlive->_device->AsGraphDevice11()->GetContext(),
				keepAlive->GetTexture2d(),
				tempHolder);
		});

	assertHr(success);
	return SUCCEEDED(success) ? &tempHolder : nullptr;
}

bool StagingTexture11::LoadFromSource(const ff::Dict& dict)
{
	return false;
}

bool StagingTexture11::LoadFromCache(const ff::Dict& dict)
{
	return false;
}

bool StagingTexture11::SaveToCache(ff::Dict& dict)
{
	ff::ComPtr<ff::ITexture> texture = Convert(ff::ConvertTextureFormat(_textureDesc.Format), _textureDesc.MipLevels);
	return ff::SaveResourceToCache(texture, dict);
}

const ff::SpriteData& StagingTexture11::GetSpriteData()
{
	if (!_spriteData)
	{
		_spriteData = std::make_unique<ff::SpriteData>();
		_spriteData->_textureView = this;
		_spriteData->_textureUV.SetRect(0, 0, 1, 1);
		_spriteData->_worldRect = ff::RectFloat(GetSize().ToType<float>());
		_spriteData->_type = GetSpriteType();
	}

	return *_spriteData;
}

void StagingTexture11::Render(
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

float StagingTexture11::GetLastFrame() const
{
	return 0;
}

float StagingTexture11::GetFPS() const
{
	return 0;
}
