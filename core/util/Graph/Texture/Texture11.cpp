#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Dict/Dict.h"
#include "Graph/DataBlob.h"
#include "Graph/DirectXUtil.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphDeviceChild.h"
#include "Graph/Anim/Animation.h"
#include "Graph/Anim/Transform.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteType.h"
#include "Graph/State/GraphContext11.h"
#include "Graph/Texture/Palette.h"
#include "Graph/Texture/PaletteData.h"
#include "Graph/Texture/PngImage.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"
#include "Thread/ThreadDispatch.h"
#include "Value/Values.h"

static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString PROP_FORMAT(L"format");
static ff::StaticString PROP_MIPS(L"mips");
static ff::StaticString PROP_PALETTE(L"palette");
static ff::StaticString PROP_SPRITE_TYPE(L"spriteType");

class __declspec(uuid("67741046-5d2f-4bf6-a955-baa950401935"))
	Texture11
	: public ff::ComBase
	, public ff::ITexture
	, public ff::ITextureDxgi
	, public ff::ITexture11
	, public ff::ITextureView
	, public ff::ITextureView11
	, public ff::IResourcePersist
	, public ff::IResourceSaveFile
	, public ff::ISprite
	, public ff::IAnimation
	, public ff::IAnimationPlayer
{
public:
	DECLARE_HEADER(Texture11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(DirectX::ScratchImage&& data, ff::IPaletteData* paletteData);
	bool Init(const D3D11_TEXTURE2D_DESC& desc, ff::IPaletteData* paletteData);

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
	virtual ff::IPalette* GetPalette() const override;
	virtual ff::ComPtr<ff::ITextureView> CreateView(size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount) override;
	virtual ff::ComPtr<ff::ITexture> Convert(ff::TextureFormat format, size_t mips) override;
	virtual void Update(size_t arrayIndex, size_t mipIndex, const ff::RectSize& rect, const void* data, ff::TextureFormat dataFormat, bool updateLocalCache) override;
	virtual ff::ISprite* AsSprite() override;
	virtual ff::ITextureView* AsTextureView() override;
	virtual ff::ITextureDxgi* AsTextureDxgi() override;
	virtual ff::ITexture11* AsTexture11() override;

	// ITextureDxgi
	virtual DXGI_FORMAT GetDxgiFormat() const override;
	virtual std::shared_ptr<DirectX::ScratchImage> Capture(bool useLocalCache = true) override;

	// ITexture11
	virtual ID3D11Texture2D* GetTexture2d() override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

	// IResourceSaveFile
	virtual ff::String GetFileExtension() const override;
	virtual bool SaveToFile(ff::StringRef file) override;

	// ISprite
	virtual const ff::SpriteData& GetSpriteData() override;

	// IAnimation
	virtual float GetFrameLength() const override;
	virtual float GetFramesPerSecond() const override;
	virtual void GetFrameEvents(float start, float end, bool includeStart, ff::ItemCollector<ff::AnimationEvent>& events) override;
	virtual void RenderFrame(ff::IRendererActive* render, const ff::Transform& position, float frame, const ff::Dict* params) override;
	virtual ff::ValuePtr GetFrameValue(ff::hash_t name, float frame, const ff::Dict* params) override;
	virtual ff::ComPtr<ff::IAnimationPlayer> CreateAnimationPlayer(float startFrame, float speed, const ff::Dict* params) override;

	// IAnimationPlayer
	virtual void AdvanceAnimation(ff::ItemCollector<ff::AnimationEvent>* frameEvents) override;
	virtual void RenderAnimation(ff::IRendererActive* render, const ff::Transform& position) override;
	virtual float GetCurrentFrame() const override;
	virtual ff::IAnimation* GetAnimation() override;

private:
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<ID3D11Texture2D> _texture;
	ff::ComPtr<ID3D11ShaderResourceView> _view;
	ff::ComPtr<ff::IPalette> _palette;
	ff::SpriteType _spriteType;
	std::unique_ptr<ff::SpriteData> _spriteData;
	std::shared_ptr<DirectX::ScratchImage> _scratch;
	std::unique_ptr<D3D11_TEXTURE2D_DESC> _desc;
};

BEGIN_INTERFACES(Texture11)
	HAS_INTERFACE(ff::ITexture)
	HAS_INTERFACE(ff::ITextureView)
	HAS_INTERFACE(ff::IResourcePersist)
	HAS_INTERFACE(ff::IResourceSaveFile)
	HAS_INTERFACE(ff::ISprite)
	HAS_INTERFACE(ff::IAnimation)
	HAS_INTERFACE(ff::IAnimationPlayer)
END_INTERFACES()

static ff::ModuleStartup RegisterTexture([](ff::Module& module)
	{
		module.RegisterClassT<Texture11>(L"texture");
	});

bool CreateTexture11(ff::IGraphDevice* device, DirectX::ScratchImage&& data, ff::IPaletteData* paletteData, ff::ITexture** texture)
{
	assertRetVal(texture, false);

	ff::ComPtr<Texture11, ff::ITexture> obj;
	assertHrRetVal(ff::ComAllocator<Texture11>::CreateInstance(device, &obj), false);
	assertRetVal(obj->Init(std::move(data), paletteData), false);

	*texture = obj.Detach();
	return true;
}

bool CreateTexture11(ff::IGraphDevice* device, ff::StringRef path, DXGI_FORMAT format, size_t mips, ff::ITexture** texture)
{
	assertRetVal(texture, false);

	DirectX::ScratchImage paletteScratch;
	DirectX::ScratchImage data = ff::LoadTextureData(path, format, mips, &paletteScratch);
	assertRetVal(data.GetImageCount(), false);

	ff::ComPtr<ff::IPaletteData> paletteData;
	if (paletteScratch.GetImageCount())
	{
		assertRetVal(ff::CreatePaletteData(device, std::move(paletteScratch), &paletteData), false);
	}

	ff::ComPtr<ff::ITexture> obj;
	assertRetVal(::CreateTexture11(device, std::move(data), paletteData, &obj), false);

	*texture = obj.Detach();
	return true;
}

bool CreateTexture11(ff::IGraphDevice* device, const D3D11_TEXTURE2D_DESC& desc, ff::IPaletteData* paletteData, ff::ITexture** texture)
{
	assertRetVal(device, false);

	ff::ComPtr<Texture11, ff::ITexture> obj;
	assertHrRetVal(ff::ComAllocator<Texture11>::CreateInstance(device, &obj), false);
	assertRetVal(obj->Init(desc, paletteData), false);

	*texture = obj.Detach();
	return true;
}

Texture11::Texture11()
	: _spriteType(ff::SpriteType::Unknown)
{
}

Texture11::~Texture11()
{
	if (_device)
	{
		_device->RemoveChild(static_cast<ff::ITexture*>(this));
	}
}

HRESULT Texture11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(static_cast<ff::ITexture*>(this));

	return ff::ComBase::_Construct(unkOuter);
}

bool Texture11::Init(DirectX::ScratchImage&& data, ff::IPaletteData* paletteData)
{
	assertRetVal(data.GetImageCount(), false);
	_scratch = std::make_shared<DirectX::ScratchImage>(std::move(data));
	_spriteType = ff::GetSpriteTypeForImage(*_scratch);

	if (paletteData && GetDxgiFormat() == DXGI_FORMAT_R8_UINT)
	{
		_palette = paletteData->CreatePalette();
	}

	return true;
}

bool Texture11::Init(const D3D11_TEXTURE2D_DESC& desc, ff::IPaletteData* paletteData)
{
	bool usesPalette = (desc.Format == DXGI_FORMAT_R8_UINT);

	_desc = std::make_unique<D3D11_TEXTURE2D_DESC>(desc);
	_spriteType = usesPalette ? ff::SpriteType::OpaquePalette : (DirectX::HasAlpha(desc.Format) ? ff::SpriteType::Transparent : ff::SpriteType::Opaque);

	if (paletteData && usesPalette)
	{
		_palette = paletteData->CreatePalette();
	}

	return true;
}

ff::IGraphDevice* Texture11::GetDevice() const
{
	return _device;
}

bool Texture11::Reset()
{
	_texture = nullptr;
	_view = nullptr;
	return true;
}

ff::PointInt Texture11::GetSize() const
{
	if (_scratch)
	{
		const DirectX::TexMetadata& md = _scratch->GetMetadata();
		return ff::PointSize(md.width, md.height).ToType<int>();
	}

	if (_desc)
	{
		return ff::PointType<unsigned int>(_desc->Width, _desc->Height).ToType<int>();
	}

	assertRetVal(false, ff::PointInt::Zeros());
}

size_t Texture11::GetMipCount() const
{
	if (_scratch)
	{
		const DirectX::TexMetadata& md = _scratch->GetMetadata();
		return md.mipLevels;
	}

	if (_desc)
	{
		return _desc->MipLevels;
	}

	assertRetVal(false, 0);
}

size_t Texture11::GetArraySize() const
{
	if (_scratch)
	{
		const DirectX::TexMetadata& md = _scratch->GetMetadata();
		return md.arraySize;
	}

	if (_desc)
	{
		return _desc->ArraySize;
	}

	assertRetVal(false, 0);
}

size_t Texture11::GetSampleCount() const
{
	if (_scratch)
	{
		return 1;
	}

	if (_desc)
	{
		return _desc->SampleDesc.Count;
	}

	assertRetVal(false, 0);
}

ff::TextureFormat Texture11::GetFormat() const
{
	return ff::ConvertTextureFormat(GetDxgiFormat());
}

ff::SpriteType Texture11::GetSpriteType() const
{
	return _spriteType;
}

ff::IPalette* Texture11::GetPalette() const
{
	return _palette;
}

bool CreateTextureView11(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount, ff::ITextureView** obj);

ff::ComPtr<ff::ITextureView> Texture11::CreateView(size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount)
{
	ff::ComPtr<ff::ITextureView> obj;
	return ::CreateTextureView11(this, arrayStart, arrayCount, mipStart, mipCount, &obj) ? obj : nullptr;
}

ff::ISprite* Texture11::AsSprite()
{
	return this;
}

ff::ITextureView* Texture11::AsTextureView()
{
	return this;
}

ff::ITextureDxgi* Texture11::AsTextureDxgi()
{
	return this;
}

ff::ITexture11* Texture11::AsTexture11()
{
	return this;
}

DXGI_FORMAT Texture11::GetDxgiFormat() const
{
	if (_scratch)
	{
		const DirectX::TexMetadata& md = _scratch->GetMetadata();
		return md.format;
	}

	if (_desc)
	{
		return _desc->Format;
	}

	assertRetVal(false, DXGI_FORMAT_UNKNOWN);
}

ff::ITexture* Texture11::GetTexture()
{
	return this;
}

ID3D11Texture2D* Texture11::GetTexture2d()
{
	if (!_texture)
	{
		if (_scratch)
		{
			ff::ComPtr<ID3D11Resource> resource;
			assertHrRetVal(DirectX::CreateTexture(
				_device->AsGraphDevice11()->Get3d(),
				_scratch->GetImages(),
				_scratch->GetImageCount(),
				_scratch->GetMetadata(),
				&resource), nullptr);

			assertRetVal(_texture.QueryFrom(resource), nullptr);
		}
		else if (_desc)
		{
			assertHrRetVal(_device->AsGraphDevice11()->Get3d()->CreateTexture2D(_desc.get(), nullptr, &_texture), false);
		}
		else
		{
			assert(false);
		}
	}

	return _texture;
}

ff::ITextureView11* Texture11::AsTextureView11()
{
	return this;
}

ID3D11ShaderResourceView* Texture11::GetView()
{
	if (!_view)
	{
		_view = ff::CreateDefaultTextureView(_device->AsGraphDevice11()->Get3d(), GetTexture2d());
	}

	return _view;
}

ff::ComPtr<ff::ITexture> Texture11::Convert(ff::TextureFormat format, size_t mips)
{
	DXGI_FORMAT dxgiFormat = ff::ConvertTextureFormat(format);
	if (GetDxgiFormat() == dxgiFormat && mips && GetMipCount() >= mips)
	{
		return this;
	}

	std::shared_ptr<DirectX::ScratchImage> scratch = Capture();
	DirectX::ScratchImage data = ff::ConvertTextureData(*scratch, dxgiFormat, mips);
	return _device->AsGraphDeviceInternal()->CreateTexture(std::move(data), nullptr);
}

void Texture11::Update(size_t arrayIndex, size_t mipIndex, const ff::RectSize& rect, const void* data, ff::TextureFormat dataFormat, bool updateLocalCache)
{
	assertRet(GetFormat() == dataFormat);

	size_t rowPitch, slicePitch;
	DirectX::ComputePitch(GetDxgiFormat(), rect.Width(), rect.Height(), rowPitch, slicePitch);

	if (updateLocalCache || !_texture)
	{
		std::shared_ptr<DirectX::ScratchImage> scratch = Capture();

		DirectX::Image image{};
		image.width = rect.Width();
		image.height = rect.Height();
		image.format = GetDxgiFormat();
		image.rowPitch = rowPitch;
		image.slicePitch = slicePitch;
		image.pixels = (BYTE*)const_cast<void*>(data);
		verifyHr(DirectX::CopyRectangle(image, DirectX::Rect(0, 0, image.width, image.height), *scratch->GetImage(mipIndex, arrayIndex, 0), 0, rect.left, rect.top));
	}

	if (_texture)
	{
		CD3D11_BOX box((UINT)rect.left, (UINT)rect.top, 0, (UINT)rect.right, (UINT)rect.bottom, 1);
		UINT subResource = ::D3D11CalcSubresource((UINT)mipIndex, (UINT)arrayIndex, (UINT)GetMipCount());
		_device->AsGraphDevice11()->GetStateContext().UpdateSubresource(_texture, subResource, &box, data, (UINT)rowPitch, 0);
	}
}

std::shared_ptr<DirectX::ScratchImage> Texture11::Capture(bool useLocalCache)
{
	if (!_scratch || !useLocalCache)
	{
		DirectX::ScratchImage scratch;
		HRESULT hr = E_FAIL;

		if (_texture)
		{
			// Can't use the device context on background threads
			ff::GetGameThreadDispatch()->Send([this, &scratch, &hr]()
				{
					ff::IGraphDevice11* device = _device->AsGraphDevice11();
					hr = DirectX::CaptureTexture(device->Get3d(), device->GetContext(), _texture, scratch);
				});
		}

		if (FAILED(hr))
		{
			ff::PointSize size = GetSize().ToType<size_t>();
			verifyHr(scratch.Initialize2D(GetDxgiFormat(), size.x, size.y, GetArraySize(), GetMipCount()));
			std::memset(scratch.GetPixels(), 0, scratch.GetPixelsSize());
		}

		_scratch = std::make_shared<DirectX::ScratchImage>(std::move(scratch));
	}

	return _scratch;
}

bool Texture11::LoadFromSource(const ff::Dict& dict)
{
	size_t mipsProp = dict.Get<ff::SizeValue>(PROP_MIPS, 1);
	ff::String fullFile = dict.Get<ff::StringValue>(PROP_FILE);
	DXGI_FORMAT format = ff::ParseDxgiTextureFormat(dict.Get<ff::StringValue>(PROP_FORMAT, ff::String(L"rgba32")));
	assertRetVal(format != DXGI_FORMAT_UNKNOWN, false);

	DirectX::ScratchImage paletteScratch;
	DirectX::ScratchImage data = ff::LoadTextureData(fullFile, format, mipsProp, &paletteScratch);

	ff::ComPtr<ff::IPaletteData> paletteData;
	if (paletteScratch.GetImageCount())
	{
		assertRetVal(ff::CreatePaletteData(_device, std::move(paletteScratch), &paletteData), false);
	}

	assertRetVal(Init(std::move(data), paletteData), false);

	return true;
}

bool Texture11::LoadFromCache(const ff::Dict& dict)
{
	_spriteType = (ff::SpriteType)dict.Get<ff::IntValue>(PROP_SPRITE_TYPE);

	ff::ComPtr<ff::IPaletteData> paletteData;
	if (paletteData.QueryFrom(dict.Get<ff::ObjectValue>(PROP_PALETTE)))
	{
		_palette = paletteData->CreatePalette();
	}

	ff::ComPtr<ff::IData> data = dict.Get<ff::DataValue>(PROP_DATA);
	DirectX::ScratchImage scratch;
	assertHrRetVal(DirectX::LoadFromDDSMemory(data->GetMem(), data->GetSize(), DirectX::DDS_FLAGS_NONE, nullptr, scratch), false);
	_scratch = std::make_shared<DirectX::ScratchImage>(std::move(scratch));

	return true;
}

bool Texture11::SaveToCache(ff::Dict& dict)
{
	std::shared_ptr<DirectX::ScratchImage> scratch = Capture();
	DirectX::Blob blob;
	assertHrRetVal(DirectX::SaveToDDSMemory(scratch->GetImages(), scratch->GetImageCount(), scratch->GetMetadata(), DirectX::DDS_FLAGS_NONE, blob), false);

	ff::ComPtr<ff::IData> blobData;
	assertRetVal(ff::CreateDataFromBlob(std::move(blob), &blobData), false);
	dict.Set<ff::DataValue>(PROP_DATA, blobData);
	dict.Set<ff::IntValue>(PROP_SPRITE_TYPE, (int)_spriteType);

	if (_palette)
	{
		dict.Set<ff::ObjectValue>(PROP_PALETTE, _palette->GetData());
	}

	return true;
}

ff::String Texture11::GetFileExtension() const
{
	return ff::String::from_static(L".png");
}

bool Texture11::SaveToFile(ff::StringRef file)
{
	std::shared_ptr<DirectX::ScratchImage> scratch = Capture();
	std::shared_ptr<DirectX::ScratchImage> paletteScratch = _palette ? _palette->GetData()->GetTexture()->AsTextureDxgi()->Capture() : nullptr;

	if (scratch->GetMetadata().format != DXGI_FORMAT_R8_UINT)
	{
		*scratch = ff::ConvertTextureData(*scratch, DXGI_FORMAT_R8G8B8A8_UNORM, GetMipCount());
	}

	for (size_t i = 0; i < scratch->GetImageCount(); i++)
	{
		ff::String file2 = file;
		ff::ChangePathExtension(file2, ff::String::format_new(L".%lu.png", i));

		ff::ComPtr<ff::IDataFile> dataFile;
		ff::ComPtr<ff::IDataWriter> writer;
		assertRetVal(ff::CreateDataFile(file2, false, &dataFile), false);
		assertRetVal(ff::CreateDataWriter(dataFile, 0, &writer), false);

		ff::PngImageWriter png(writer);
		png.Write(scratch->GetImages()[i], paletteScratch ? paletteScratch->GetImages() : nullptr);
	}

	return true;
}

const ff::SpriteData& Texture11::GetSpriteData()
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

float Texture11::GetFrameLength() const
{
	return 0;
}

float Texture11::GetFramesPerSecond() const
{
	return 0;
}

void Texture11::GetFrameEvents(float start, float end, bool includeStart, ff::ItemCollector<ff::AnimationEvent>& events)
{
}

void Texture11::RenderFrame(ff::IRendererActive* render, const ff::Transform& position, float frame, const ff::Dict* params)
{
	render->DrawSprite(this, position);
}

ff::ValuePtr Texture11::GetFrameValue(ff::hash_t name, float frame, const ff::Dict* params)
{
	return nullptr;
}

ff::ComPtr<ff::IAnimationPlayer> Texture11::CreateAnimationPlayer(float startFrame, float speed, const ff::Dict* params)
{
	return this;
}

void Texture11::AdvanceAnimation(ff::ItemCollector<ff::AnimationEvent>* frameEvents)
{
}

void Texture11::RenderAnimation(ff::IRendererActive* render, const ff::Transform& position)
{
	render->DrawSprite(this, position);
}

float Texture11::GetCurrentFrame() const
{
	return 0;
}

ff::IAnimation* Texture11::GetAnimation()
{
	return this;
}
