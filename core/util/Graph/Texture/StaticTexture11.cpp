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
#include "Value/Values.h"

static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString PROP_FORMAT(L"format");
static ff::StaticString PROP_MIPS(L"mips");
static ff::StaticString PROP_PALETTE(L"palette");
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
	, public ff::IResourceSaveFile
	, public ff::ISprite
	, public ff::IAnimation
	, public ff::IAnimationPlayer
{
public:
	DECLARE_HEADER(StaticTexture11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(DirectX::ScratchImage&& data, ff::IPaletteData* paletteData);

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
	std::unique_ptr<ff::SpriteData> _spriteData;
	DirectX::ScratchImage _originalData;
	ff::SpriteType _spriteType;
};

BEGIN_INTERFACES(StaticTexture11)
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
		module.RegisterClassT<StaticTexture11>(L"texture");
	});

bool CreateTexture11(ff::IGraphDevice* device, DirectX::ScratchImage&& data, ff::IPaletteData* paletteData, ff::ITexture** texture)
{
	assertRetVal(texture, false);

	ff::ComPtr<StaticTexture11, ff::ITexture> obj;
	assertHrRetVal(ff::ComAllocator<StaticTexture11>::CreateInstance(device, &obj), false);
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

bool StaticTexture11::Init(DirectX::ScratchImage&& data, ff::IPaletteData* paletteData)
{
	assertRetVal(data.GetImageCount(), false);
	_originalData = std::move(data);
	_spriteType = ff::GetSpriteTypeForImage(_originalData);

	if (paletteData && GetDxgiFormat() == DXGI_FORMAT_R8_UINT)
	{
		_palette = paletteData->CreatePalette();
	}

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

ff::IPalette* StaticTexture11::GetPalette() const
{
	return _palette;
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
	return _device->AsGraphDeviceInternal()->CreateTexture(std::move(data), nullptr);
}

void StaticTexture11::Update(size_t arrayIndex, size_t mipIndex, const ff::RectSize& rect, const void* data, size_t rowPitch, ff::TextureFormat dataFormat)
{
	assertRet(GetFormat() == dataFormat);

	CD3D11_BOX box((UINT)rect.left, (UINT)rect.top, 0, (UINT)rect.right, (UINT)rect.bottom, 0);
	UINT subResource = ::D3D11CalcSubresource((UINT)mipIndex, (UINT)arrayIndex, (UINT)_originalData.GetMetadata().mipLevels);
	_device->AsGraphDevice11()->GetStateContext().UpdateSubresource(GetTexture2d(), subResource, &box, data, (UINT)rowPitch, 0);
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

bool StaticTexture11::LoadFromCache(const ff::Dict& dict)
{
	ff::ComPtr<ff::IData> data = dict.Get<ff::DataValue>(PROP_DATA);
	_spriteType = (ff::SpriteType)dict.Get<ff::IntValue>(PROP_SPRITE_TYPE);

	ff::ComPtr<ff::IPaletteData> paletteData;
	if (paletteData.QueryFrom(dict.Get<ff::ObjectValue>(PROP_PALETTE)))
	{
		_palette = paletteData->CreatePalette();
	}

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

	if (_palette)
	{
		dict.Set<ff::ObjectValue>(PROP_PALETTE, _palette->GetData());
	}

	return true;
}

ff::String StaticTexture11::GetFileExtension() const
{
	return ff::String::from_static(L".png");
}

bool StaticTexture11::SaveToFile(ff::StringRef file)
{
	DirectX::ScratchImage scratchHolder;
	const DirectX::ScratchImage* scratchImage = Capture(scratchHolder);

	if (scratchImage->GetMetadata().format != DXGI_FORMAT_R8_UINT)
	{
		scratchHolder = ff::ConvertTextureData(*scratchImage, DXGI_FORMAT_R8G8B8A8_UNORM, GetMipCount());
		scratchImage = &scratchHolder;
	}

	for (size_t i = 0; i < scratchImage->GetImageCount(); i++)
	{
		ff::String file2 = file;
		ff::ChangePathExtension(file2, ff::String::format_new(L".%lu.png", i));

		ff::ComPtr<ff::IDataFile> dataFile;
		ff::ComPtr<ff::IDataWriter> writer;
		assertRetVal(ff::CreateDataFile(file2, false, &dataFile), false);
		assertRetVal(ff::CreateDataWriter(dataFile, 0, &writer), false);

		DirectX::ScratchImage paletteScratchHolder;
		const DirectX::ScratchImage* paletteScratch = _palette
			? _palette->GetData()->GetTexture()->AsTextureDxgi()->Capture(paletteScratchHolder)
			: nullptr;

		ff::PngImageWriter png(writer);
		png.Write(scratchImage->GetImages()[i], paletteScratch ? paletteScratch->GetImages() : nullptr);
	}

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

float StaticTexture11::GetFrameLength() const
{
	return 0;
}

float StaticTexture11::GetFramesPerSecond() const
{
	return 0;
}

void StaticTexture11::GetFrameEvents(float start, float end, bool includeStart, ff::ItemCollector<ff::AnimationEvent>& events)
{
}

void StaticTexture11::RenderFrame(ff::IRendererActive* render, const ff::Transform& position, float frame, const ff::Dict* params)
{
	render->DrawSprite(this, position);
}

ff::ValuePtr StaticTexture11::GetFrameValue(ff::hash_t name, float frame, const ff::Dict* params)
{
	return nullptr;
}

ff::ComPtr<ff::IAnimationPlayer> StaticTexture11::CreateAnimationPlayer(float startFrame, float speed, const ff::Dict* params)
{
	return this;
}

void StaticTexture11::AdvanceAnimation(ff::ItemCollector<ff::AnimationEvent>* frameEvents)
{
}

void StaticTexture11::RenderAnimation(ff::IRendererActive* render, const ff::Transform& position)
{
	render->DrawSprite(this, position);
}

float StaticTexture11::GetCurrentFrame() const
{
	return 0;
}

ff::IAnimation* StaticTexture11::GetAnimation()
{
	return this;
}
