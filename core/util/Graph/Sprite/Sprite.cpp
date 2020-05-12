#include "pch.h"
#include "COM/ComAlloc.h"
#include "Dict/Dict.h"
#include "Graph/GraphDevice.h"
#include "Graph/Render/RenderAnimation.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteList.h"
#include "Graph/Sprite/SpriteType.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Resource/ResourceValue.h"
#include "Value/Values.h"

static ff::StaticString PROP_NAME(L"name");
static ff::StaticString PROP_SPRITES(L"sprites");

static const ff::SpriteData& GetEmptySpriteData()
{
	static const ff::SpriteData data =
	{
		ff::String(),
		nullptr,
		ff::RectFloat(0, 0, 0, 0),
		ff::RectFloat(0, 0, 0, 0),
	};

	return data;
}

ff::RectInt ff::SpriteData::GetTextureRect() const
{
	return GetTextureRectF().ToType<int>();
}

ff::RectFloat ff::SpriteData::GetTextureRectF() const
{
	assertRetVal(_textureView && _textureView->GetTexture(), RectFloat::Zeros());
	ITexture* texture = _textureView->GetTexture();

	PointFloat texSize = texture->GetSize().ToType<float>();
	RectFloat texRect = _textureUV * texSize;
	texRect = texRect.Normalize();

	return texRect;
}

ff::PointFloat ff::SpriteData::GetScale() const
{
	ff::RectFloat srcRect = GetTextureRectF();
	return ff::PointFloat(
		_worldRect.Width() / srcRect.Width(),
		_worldRect.Height() / srcRect.Height());
}

ff::PointFloat ff::SpriteData::GetHandle() const
{
	ff::PointFloat scale = GetScale();
	return ff::PointFloat(
		-_worldRect.left / scale.x,
		-_worldRect.top / scale.y);
}

class __declspec(uuid("7f942967-3605-4b36-a880-a6252d2518b9"))
	Sprite
	: public ff::ComBase
	, public ff::ISprite
	, public ff::IRenderAnimation
{
public:
	DECLARE_HEADER(Sprite);

	void Init(const ff::SpriteData& data);

	// ISprite functions
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
	ff::ComPtr<ff::ITextureView> _textureView;
	ff::SpriteData _data;
};

BEGIN_INTERFACES(Sprite)
	HAS_INTERFACE(ff::ISprite)
	HAS_INTERFACE(ff::IRenderAnimation)
END_INTERFACES()

class __declspec(uuid("778af2ef-b522-453e-b040-65dc6ea6cb93"))
	SpriteResource
	: public ff::ComBase
	, public ff::ISpriteResource
	, public ff::IRenderAnimation
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(SpriteResource);

	bool Init(ff::SharedResourceValue spriteOrListRes, ff::StringRef name);

	// ISprite
	virtual const ff::SpriteData& GetSpriteData() override;

	// ISpriteResource
	virtual ff::SharedResourceValue GetSourceResource() override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

	// IRenderAnimation
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
	ff::String _name;
	ff::AutoResourceValue _spriteRes;
	ff::ComPtr<ff::ISprite> _sprite;
	const ff::SpriteData* _data;
};

BEGIN_INTERFACES(SpriteResource)
	HAS_INTERFACE(ff::ISprite)
	HAS_INTERFACE(ff::ISpriteResource)
	HAS_INTERFACE(ff::IRenderAnimation)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<Sprite>(L"spriteInList");
		module.RegisterClassT<SpriteResource>(L"sprite");
	});

bool ff::CreateSprite(const SpriteData& data, ISprite** ppSprite)
{
	assertRetVal(ppSprite && data._textureView && !data._worldRect.IsEmpty(), false);

	ComPtr<Sprite, ISprite> myObj;
	assertHrRetVal(ff::ComAllocator<Sprite>::CreateInstance(&myObj), false);
	myObj->Init(data);

	*ppSprite = myObj.Detach();
	return true;
}

bool ff::CreateSprite(ITextureView* textureView, StringRef name, RectFloat rect, PointFloat handle, PointFloat scale, SpriteType type, ISprite** ppSprite)
{
	assertRetVal(ppSprite && textureView, false);

	ff::ITexture* texture = textureView->GetTexture();
	PointInt sizeTex = texture->GetSize();
	assertRetVal(sizeTex.x && sizeTex.y, false);

	SpriteData data;
	data._name = name;
	data._textureView = textureView;
	data._type = (type == ff::SpriteType::Unknown) ? texture->GetSpriteType() : type;

	data._textureUV.SetRect(
		rect.left / sizeTex.x,
		rect.top / sizeTex.y,
		rect.right / sizeTex.x,
		rect.bottom / sizeTex.y);

	data._worldRect.SetRect(
		-handle.x * scale.x,
		-handle.y * scale.y,
		(-handle.x + rect.Width()) * scale.x,
		(-handle.y + rect.Height()) * scale.y);

	return CreateSprite(data, ppSprite);
}

bool ff::CreateSpriteResource(SharedResourceValue spriteOrListRes, ISprite** obj)
{
	return ff::CreateSpriteResource(spriteOrListRes, ff::GetEmptyString(), obj);
}

bool ff::CreateSpriteResource(SharedResourceValue spriteOrListRes, StringRef name, ISprite** obj)
{
	assertRetVal(obj, false);

	ff::ComPtr<SpriteResource, ISprite> myObj;
	assertHrRetVal(ff::ComAllocator<SpriteResource>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(spriteOrListRes, name), false);

	*obj = myObj.Detach();
	return true;
}

bool ff::CreateSpriteResource(SharedResourceValue spriteOrListRes, size_t index, ISprite** obj)
{
	String name = String::format_new(L"%lu", index);
	return CreateSpriteResource(spriteOrListRes, name, obj);
}

Sprite::Sprite()
{
}

Sprite::~Sprite()
{
}

const ff::SpriteData& Sprite::GetSpriteData()
{
	return _data;
}

void Sprite::Render(
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

float Sprite::GetLastFrame() const
{
	return 0;
}

float Sprite::GetFPS() const
{
	return 0;
}

void Sprite::Init(const ff::SpriteData& data)
{
	_textureView = data._textureView;
	_data = data;

	if (_data._type == ff::SpriteType::Unknown)
	{
		_data._type = _textureView->GetTexture()->GetSpriteType();
	}
}

SpriteResource::SpriteResource()
	: _data(nullptr)
{
}

SpriteResource::~SpriteResource()
{
}

bool SpriteResource::Init(ff::SharedResourceValue spriteOrListRes, ff::StringRef name)
{
	assertRetVal(spriteOrListRes, false);
	_spriteRes.Init(spriteOrListRes);
	_name = name;

	return true;
}

const ff::SpriteData& SpriteResource::GetSpriteData()
{
	if (_data)
	{
		return *_data;
	}

	ff::ValuePtr value = _spriteRes.GetValue();
	if (value)
	{
		if (value->IsType<ff::ObjectValue>())
		{
			ff::ComPtr<ff::ISprite> sprite;
			ff::ComPtr<ff::ISpriteList> sprites;

			if (sprites.QueryFrom(value->GetValue<ff::ObjectValue>()))
			{
				_sprite = sprites->Get(_name);
			}
			else if (sprite.QueryFrom(value->GetValue<ff::ObjectValue>()))
			{
				_sprite = sprite;
			}
		}

		if (!value->IsType<ff::NullValue>())
		{
			assert(_sprite);
			if (_sprite)
			{
				// no need to cache empty sprite data, it's temporary
				const ff::SpriteData& data = _sprite->GetSpriteData();
				if (&data != &GetEmptySpriteData())
				{
					_data = &data;
				}
			}
		}
	}

	return _data ? *_data : GetEmptySpriteData();
}

ff::SharedResourceValue SpriteResource::GetSourceResource()
{
	return _spriteRes.GetResourceValue();
}

bool SpriteResource::LoadFromSource(const ff::Dict& dict)
{
	return LoadFromCache(dict);
}

bool SpriteResource::LoadFromCache(const ff::Dict& dict)
{
	_spriteRes.Init(dict.Get<ff::SharedResourceWrapperValue>(PROP_SPRITES));
	_name = dict.Get<ff::StringValue>(PROP_NAME);

	return true;
}

bool SpriteResource::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::SharedResourceWrapperValue>(PROP_SPRITES, _spriteRes.GetResourceValue());
	dict.Set<ff::StringValue>(PROP_NAME, ff::String(_name));

	return true;
}

void SpriteResource::Render(
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

float SpriteResource::GetLastFrame() const
{
	return 0;
}

float SpriteResource::GetFPS() const
{
	return 0;
}
