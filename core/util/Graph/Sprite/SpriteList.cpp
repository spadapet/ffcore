#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Dict/Dict.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteList.h"
#include "Graph/Sprite/SpriteOptimizer.h"
#include "Graph/GraphDevice.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Texture/TextureView.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Resource/ResourceValue.h"
#include "String/StringUtil.h"
#include "Value/Values.h"
#include "Windows/FileUtil.h"

static ff::StaticString PROP_COUNT(L"count");
static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString PROP_FORMAT(L"format");
static ff::StaticString PROP_HANDLE(L"handle");
static ff::StaticString PROP_MIPS(L"mips");
static ff::StaticString PROP_REPEAT(L"repeat");
static ff::StaticString PROP_OFFSET(L"offset");
static ff::StaticString PROP_OPTIMIZE(L"optimize");
static ff::StaticString PROP_POS(L"pos");
static ff::StaticString PROP_SCALE(L"scale");
static ff::StaticString PROP_SIZE(L"size");
static ff::StaticString PROP_SPRITES(L"sprites");
static ff::StaticString PROP_TEXTURES(L"textures");

class __declspec(uuid("7ddb9bd1-c9e0-4788-b0b2-3bb252515013"))
	SpriteList
	: public ff::ComBase
	, public ff::ISpriteList
	, public ff::IResourcePersist
	, public ff::IResourceSaveSiblings
{
public:
	DECLARE_HEADER(SpriteList);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;

	// IGraphDeviceChild functions
	virtual ff::IGraphDevice* GetDevice() const override;
	virtual bool Reset() override;

	// ISpriteList functions
	virtual ff::ISprite* Add(ff::ITextureView* texture, ff::StringRef name, ff::RectFloat rect, ff::PointFloat handle, ff::PointFloat scale, ff::SpriteType type) override;
	virtual ff::ISprite* Add(ff::ISprite* sprite) override;
	virtual bool Add(ff::ISpriteList* list) override;

	virtual size_t GetCount() override;
	virtual ff::ISprite* Get(size_t index) override;
	virtual ff::ISprite* Get(ff::StringRef name) override;
	virtual ff::StringRef GetName(size_t index) override;
	virtual size_t GetIndex(ff::StringRef name) override;
	virtual bool Remove(ff::ISprite* sprite) override;
	virtual bool Remove(size_t index) override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

	// IResourceSaveSiblings
	virtual ff::Dict GetSiblingResources(ff::SharedResourceValue parentValue) override;

private:
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::Vector<ff::ComPtr<ff::ISprite>> _sprites;
};

BEGIN_INTERFACES(SpriteList)
	HAS_INTERFACE(ff::ISpriteList)
	HAS_INTERFACE(ff::IResourcePersist)
	HAS_INTERFACE(ff::IResourceSaveSiblings)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<SpriteList>(L"sprites");
	});

bool ff::CreateSpriteList(ff::IGraphDevice* device, ISpriteList** list)
{
	return SUCCEEDED(ComAllocator<SpriteList>::CreateInstance(device, GUID_NULL, __uuidof(ISpriteList), (void**)list));
}

SpriteList::SpriteList()
{
}

SpriteList::~SpriteList()
{
	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT SpriteList::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

ff::IGraphDevice* SpriteList::GetDevice() const
{
	return _device;
}

bool SpriteList::Reset()
{
	return true;
}

ff::ISprite* SpriteList::Add(ff::ITextureView* texture, ff::StringRef name, ff::RectFloat rect, ff::PointFloat handle, ff::PointFloat scale, ff::SpriteType type)
{
	ff::ComPtr<ff::ISprite> sprite;
	assertRetVal(ff::CreateSprite(texture, name, rect, handle, scale, type, &sprite), false);

	_sprites.Push(sprite);

	return sprite;
}

ff::ISprite* SpriteList::Add(ff::ISprite* sprite)
{
	assertRetVal(sprite, nullptr);

	ff::ComPtr<ff::ISprite> pNewSprite;
	assertRetVal(CreateSprite(sprite->GetSpriteData(), &pNewSprite), false);

	_sprites.Push(pNewSprite);

	return pNewSprite;
}

bool SpriteList::Add(ISpriteList* list)
{
	assertRetVal(list && list->GetDevice() == _device, false);

	for (size_t i = 0; i < list->GetCount(); i++)
	{
		Add(list->Get(i));
	}

	return true;
}

size_t SpriteList::GetCount()
{
	return _sprites.Size();
}

ff::ISprite* SpriteList::Get(size_t index)
{
	noAssertRetVal(index < _sprites.Size(), nullptr);

	return _sprites[index];
}

ff::ISprite* SpriteList::Get(ff::StringRef name)
{
	size_t nIndex = GetIndex(name);

	if (nIndex == ff::INVALID_SIZE && name.size() && isdigit(name[0]))
	{
		int nValue = 0;
		int nChars = 0;

		// convert the string to an integer
		if (_snwscanf_s(name.c_str(), name.size(), L"%d%n", &nValue, &nChars) == 1 &&
			(size_t)nChars == name.size())
		{
			return Get((size_t)nValue);
		}
	}

	return (nIndex != ff::INVALID_SIZE) ? _sprites[nIndex] : nullptr;
}

ff::StringRef SpriteList::GetName(size_t index)
{
	assertRetVal(index < GetCount(), ff::GetEmptyString());
	return _sprites[index]->GetSpriteData()._name;
}

size_t SpriteList::GetIndex(ff::StringRef name)
{
	assertRetVal(name.size(), ff::INVALID_SIZE);
	size_t nLen = name.size();

	for (size_t i = 0; i < _sprites.Size(); i++)
	{
		const ff::SpriteData& data = _sprites[i]->GetSpriteData();

		if (data._name.size() == nLen && data._name == name)
		{
			return i;
		}
	}

	return ff::INVALID_SIZE;
}

bool SpriteList::Remove(ff::ISprite* sprite)
{
	for (size_t i = 0; i < _sprites.Size(); i++)
	{
		if (_sprites[i] == sprite)
		{
			_sprites.Delete(i);
			return true;
		}
	}

	assertRetVal(false, false);
}

bool SpriteList::Remove(size_t index)
{
	assertRetVal(index >= 0 && index < _sprites.Size(), false);
	_sprites.Delete(index);

	return true;
}

bool SpriteList::LoadFromSource(const ff::Dict& dict)
{
	bool optimize = dict.Get<ff::BoolValue>(PROP_OPTIMIZE, true);
	size_t mips = dict.Get<ff::SizeValue>(PROP_MIPS, 1);
	ff::String formatProp = dict.Get<ff::StringValue>(PROP_FORMAT, ff::String(L"rgba32"));
	ff::TextureFormat format = ff::ParseTextureFormat(formatProp);
	assertRetVal(format != ff::TextureFormat::Unknown, nullptr);

	ff::Dict spritesDict = dict.Get<ff::DictValue>(PROP_SPRITES);
	ff::Vector<ff::String> names = spritesDict.GetAllNames(true);
	ff::Map<ff::String, ff::ComPtr<ff::ITextureView>> textureViews;

	ff::ComPtr<ff::ISpriteList> origSprites;
	assertRetVal(ff::CreateSpriteList(_device, &origSprites), false);

	for (ff::StringRef name : names)
	{
		ff::Dict spriteDict = spritesDict.Get<ff::DictValue>(name);

		ff::PointFloat pos;
		ff::PointFloat size;
		ff::RectFloat posRect = spriteDict.Get<ff::RectFloatValue>(PROP_POS);
		if (!posRect.IsZeros())
		{
			pos = posRect.TopLeft();
			size = posRect.Size();
		}
		else
		{
			pos = spriteDict.Get<ff::PointFloatValue>(PROP_POS);
			size = spriteDict.Get<ff::PointFloatValue>(PROP_SIZE);
		}

		ff::PointFloat offset = spriteDict.Get<ff::PointFloatValue>(PROP_OFFSET, ff::PointFloat(size.x, 0));
		ff::String fullFile = spriteDict.Get<ff::StringValue>(PROP_FILE);
		ff::PointFloat handle = spriteDict.Get<ff::PointFloatValue>(PROP_HANDLE);
		ff::PointFloat scale = spriteDict.Get<ff::PointFloatValue>(PROP_SCALE, ff::PointFloat(1, 1));
		size_t repeat = spriteDict.Get<ff::SizeValue>(PROP_REPEAT, 1);

		ff::ComPtr<ff::ITextureView> textureView;
		auto iter = textureViews.GetKey(fullFile);
		if (iter)
		{
			textureView = iter->GetValue();
		}
		else
		{
			ff::ComPtr<ff::ITexture> texture = _device->CreateTexture(fullFile,
				optimize ? ff::TextureFormat::RGBA32 : format,
				optimize ? 1 : mips);
			assertRetVal(texture, false);

			textureView = texture->AsTextureView();
			assertRetVal(textureView, false);
			textureViews.SetKey(fullFile, textureView);
		}

		if (size.x == 0 && size.y == 0 && handle.x == 0 && handle.y == 0)
		{
			size = textureView->GetTexture()->GetSize().ToType<float>();
		}

		if (size.x < 1 || size.y < 1)
		{
			return nullptr;
		}

		for (size_t i = 0; i < repeat; i++)
		{
			ff::String spriteName = (repeat > 1)
				? ff::String::format_new(L"%s %d", name.c_str(), i)
				: name;
			ff::PointFloat topLeft(pos.x + offset.x * i, pos.y + offset.y * i);
			ff::RectFloat rect(topLeft, topLeft + size);

			assertRetVal(origSprites->Add(textureView, spriteName, rect, handle, scale, ff::SpriteType::Unknown), false);
		}
	}

	if (optimize)
	{
		ff::ISpriteList* finalSprites = this;
		assertRetVal(ff::OptimizeSprites(origSprites, format, mips, &finalSprites), false);
	}
	else for (size_t i = 0; i < origSprites->GetCount(); i++)
	{
		Add(origSprites->Get(i));
	}

	return true;
}

ff::Dict SpriteList::GetSiblingResources(ff::SharedResourceValue parentValue)
{
	ff::Dict siblings;

	for (size_t i = 0; i < GetCount(); i++)
	{
		ff::ComPtr<ff::ISprite> sprite;
		if (ff::CreateSpriteResource(parentValue, i, &sprite))
		{
			ff::ValuePtr spriteValue = ff::Value::New<ff::ObjectValue>(sprite);
			ff::String spriteName = ff::String::format_new(L"%s.%s", parentValue->GetName().c_str(), GetName(i).c_str());
			siblings.SetValue(spriteName, spriteValue);
		}
	}

	return siblings;
}

bool SpriteList::LoadFromCache(const ff::Dict& dict)
{
	size_t count = dict.Get<ff::SizeValue>(PROP_COUNT);
	ff::ComPtr<ff::IData> spritesData = dict.Get<ff::DataValue>(PROP_SPRITES);
	ff::ValuePtr textureVectorValue = dict.GetValue(PROP_TEXTURES);
	assertRetVal(spritesData && textureVectorValue->IsType<ff::ValueVectorValue>(), false);

	ff::Vector<ff::ComPtr<ff::ITextureView>> textureViews;
	for (ff::ValuePtr value : textureVectorValue->GetValue<ff::ValueVectorValue>())
	{
		ff::ComPtr<ff::ITexture> texture;
		assertRetVal(value->IsType<ff::ObjectValue>(), false);
		assertRetVal(texture.QueryFrom(value->GetValue<ff::ObjectValue>()) && texture->AsTextureView(), false);
		textureViews.Push(texture->AsTextureView());
	}

	ff::ComPtr<ff::IDataReader> reader;
	assertRetVal(ff::CreateDataReader(spritesData, 0, &reader), false);

	for (size_t i = 0; i < count; i++)
	{
		ff::SpriteData data;
		DWORD textureIndex;
		assertRetVal(ff::LoadData(reader, textureIndex), false);
		assertRetVal(ff::LoadData(reader, data._name), false);
		assertRetVal(ff::LoadData(reader, data._textureUV), false);
		assertRetVal(ff::LoadData(reader, data._worldRect), false);
		assertRetVal(ff::LoadData(reader, data._type), false);
		assertRetVal(textureIndex < textureViews.Size(), false);
		data._textureView = textureViews[textureIndex];

		ff::ComPtr<ff::ISprite> sprite;
		assertRetVal(ff::CreateSprite(data, &sprite), false);
		assertRetVal(Add(sprite), false);
	}

	return true;
}

bool SpriteList::SaveToCache(ff::Dict& dict)
{
	// Find all unique textures
	ff::Vector<ff::ITextureView*> textures;
	for (ff::ISprite* sprite : _sprites)
	{
		const ff::SpriteData& spriteData = sprite->GetSpriteData();
		assertRetVal(spriteData._textureView, false);

		if (textures.Find(spriteData._textureView) == ff::INVALID_SIZE)
		{
			textures.Push(spriteData._textureView);
		}
	}

	// Save textures to a vector of resource dicts
	ff::ValuePtr textureVectorValue;
	{
		ff::Vector<ff::ValuePtr> textureVector;
		for (ff::ITextureView* texture : textures)
		{
			ff::Dict textureDict;
			assertRetVal(ff::SaveResourceToCache(texture, textureDict), false);
			textureVector.Push(ff::Value::New<ff::DictValue>(std::move(textureDict)));
		}

		textureVectorValue = ff::Value::New<ff::ValueVectorValue>(std::move(textureVector));
	}

	// Save each sprite
	ff::ComPtr<ff::IDataVector> spriteDataVector;
	{
		ff::ComPtr<ff::IDataWriter> writer;
		assertRetVal(ff::CreateDataWriter(&spriteDataVector, &writer), false);

		for (ff::ISprite* sprite : _sprites)
		{
			const ff::SpriteData& spriteData = sprite->GetSpriteData();
			DWORD textureIndex = (DWORD)textures.Find(spriteData._textureView);
			assertRetVal(ff::SaveData(writer, textureIndex), false);
			assertRetVal(ff::SaveData(writer, spriteData._name), false);
			assertRetVal(ff::SaveData(writer, spriteData._textureUV), false);
			assertRetVal(ff::SaveData(writer, spriteData._worldRect), false);
			assertRetVal(ff::SaveData(writer, spriteData._type), false);
		}
	}

	dict.Set<ff::SizeValue>(PROP_COUNT, _sprites.Size());
	dict.Set<ff::DataValue>(PROP_SPRITES, spriteDataVector);
	dict.SetValue(PROP_TEXTURES, textureVectorValue);

	return true;
}
