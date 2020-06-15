#include "pch.h"
#include "COM/ComAlloc.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphDeviceChild.h"
#include "Graph/Anim/Animation.h"
#include "Graph/Anim/Transform.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"

D3D_SRV_DIMENSION GetDefaultDimension(const D3D11_TEXTURE2D_DESC& desc);

class __declspec(uuid("1a29f207-5a09-46e4-9888-17d1ac2eb9be"))
	TextureView11
	: public ff::ComBase
	, public ff::ITextureView
	, public ff::ITextureView11
	, public ff::ISprite
	, public ff::IAnimation
	, public ff::IAnimationPlayer
{
public:
	DECLARE_HEADER(TextureView11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount);

	// IGraphDeviceChild
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// ITextureView
	virtual ff::ITexture* GetTexture() override;
	virtual ff::ITextureView11* AsTextureView11() override;

	// ITextureView11
	virtual ID3D11ShaderResourceView* GetView() override;

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
	ff::ComPtr<ff::ITexture> _texture;
	ff::ComPtr<ID3D11ShaderResourceView> _view;
	std::unique_ptr<ff::SpriteData> _spriteData;
	size_t _arrayStart;
	size_t _arrayCount;
	size_t _mipStart;
	size_t _mipCount;
};

BEGIN_INTERFACES(TextureView11)
	HAS_INTERFACE(ff::ITextureView)
	HAS_INTERFACE(ff::ISprite)
	HAS_INTERFACE(ff::IAnimation)
	HAS_INTERFACE(ff::IAnimationPlayer)
END_INTERFACES()

bool CreateTextureView11(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount, ff::ITextureView** obj)
{
	assertRetVal(texture && obj, false);

	ff::ComPtr<TextureView11, ff::ITextureView> myObj;
	assertHrRetVal(ff::ComAllocator<TextureView11>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(texture, arrayStart, arrayCount, mipStart, mipCount), false);

	*obj = myObj.Detach();
	return true;
}

TextureView11::TextureView11()
	: _arrayStart(0)
	, _arrayCount(0)
	, _mipStart(0)
	, _mipCount(0)
{
}

TextureView11::~TextureView11()
{
	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT TextureView11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool TextureView11::Init(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipStart, size_t mipCount)
{
	assertRetVal(texture, false);

	ID3D11Texture2D* texture2d = _texture->AsTexture11()->GetTexture2d();
	assertRetVal(texture2d, false);

	_texture = texture;
	_arrayStart = arrayStart;
	_mipStart = mipStart;

	D3D11_TEXTURE2D_DESC desc;
	texture2d->GetDesc(&desc);
	_arrayCount = arrayCount ? arrayCount : (desc.ArraySize - _arrayStart);
	_mipCount = mipCount ? mipCount : (desc.MipLevels - _mipStart);

	return true;
}

ff::IGraphDevice* TextureView11::GetDevice() const
{
	return _device;
}

bool TextureView11::Reset()
{
	_texture = nullptr;
	_view = nullptr;
	return true;
}

ff::ITexture* TextureView11::GetTexture()
{
	return _texture;
}

ff::ITextureView11* TextureView11::AsTextureView11()
{
	return this;
}

ID3D11ShaderResourceView* TextureView11::GetView()
{
	if (!_view)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		ff::ZeroObject(viewDesc);

		ID3D11Texture2D* texture2d = _texture->AsTexture11()->GetTexture2d();
		assertRetVal(texture2d, nullptr);

		D3D11_TEXTURE2D_DESC textureDesc;
		texture2d->GetDesc(&textureDesc);

		viewDesc.Format = textureDesc.Format;
		viewDesc.ViewDimension = ::GetDefaultDimension(textureDesc);

		switch (viewDesc.ViewDimension)
		{
		case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
			viewDesc.Texture2DMSArray.FirstArraySlice = (UINT)_arrayStart;
			viewDesc.Texture2DMSArray.ArraySize = (UINT)_arrayCount;
			break;

		case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
			viewDesc.Texture2DArray.FirstArraySlice = (UINT)_arrayStart;
			viewDesc.Texture2DArray.ArraySize = (UINT)_arrayCount;
			viewDesc.Texture2DArray.MostDetailedMip = (UINT)_mipStart;
			viewDesc.Texture2DArray.MipLevels = (UINT)_mipCount;
			break;

		case D3D_SRV_DIMENSION_TEXTURE2D:
			viewDesc.Texture2D.MostDetailedMip = (UINT)_mipStart;
			viewDesc.Texture2D.MipLevels = (UINT)_mipCount;
			break;
		}

		assertHrRetVal(_device->AsGraphDevice11()->Get3d()->CreateShaderResourceView(texture2d, &viewDesc, &_view), nullptr);
	}

	return _view;
}

const ff::SpriteData& TextureView11::GetSpriteData()
{
	if (!_spriteData)
	{
		_spriteData = std::make_unique<ff::SpriteData>();
		_spriteData->_textureView = this;
		_spriteData->_textureUV.SetRect(0, 0, 1, 1);
		_spriteData->_worldRect = ff::RectFloat(_texture->GetSize().ToType<float>());
		_spriteData->_type = _texture->GetSpriteType();
	}

	return *_spriteData;
}

float TextureView11::GetFrameLength() const
{
	return 0;
}

float TextureView11::GetFramesPerSecond() const
{
	return 0;
}

void TextureView11::GetFrameEvents(float start, float end, bool includeStart, ff::ItemCollector<ff::AnimationEvent>& events)
{
}

void TextureView11::RenderFrame(ff::IRendererActive* render, const ff::Transform& position, float frame, const ff::Dict* params)
{
	render->DrawSprite(this, position);
}

ff::ValuePtr TextureView11::GetFrameValue(ff::hash_t name, float frame, const ff::Dict* params)
{
	return nullptr;
}

ff::ComPtr<ff::IAnimationPlayer> TextureView11::CreateAnimationPlayer(float startFrame, float speed, const ff::Dict* params)
{
	return this;
}

void TextureView11::AdvanceAnimation(ff::ItemCollector<ff::AnimationEvent>* frameEvents)
{
}

void TextureView11::RenderAnimation(ff::IRendererActive* render, const ff::Transform& position)
{
	render->DrawSprite(this, position);
}

float TextureView11::GetCurrentFrame() const
{
	return 0;
}

ff::IAnimation* TextureView11::GetAnimation()
{
	return this;
}
