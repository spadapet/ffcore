#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Dict/Dict.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/Font/FontData.h"
#include "Graph/GraphFactory.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"
#include "Value/Values.h"
#include "Windows/FileUtil.h"

static ff::StaticString PROP_BOLD(L"bold");
static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString PROP_INDEX(L"index");
static ff::StaticString PROP_ITALIC(L"italic");

class __declspec(uuid("635b4c4f-4d55-4b5d-aa00-a197fc4cfb7a"))
	FontData
	: public ff::ComBase
	, public ff::IFontData
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(FontData);

	// IFontData functions
	virtual bool GetBold() const override;
	virtual bool GetItalic() const override;
	virtual size_t GetIndex() const override;
	virtual ff::IData* GetData() const override;
	virtual IDWriteFontFaceX* GetFontFace() override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	ff::Mutex _mutex;
	bool _bold;
	bool _italic;
	bool _createdFontFace;
	size_t _index;
	ff::ComPtr<ff::IData> _data;
	ff::ComPtr<IDWriteFontFaceX> _fontFace;
};

BEGIN_INTERFACES(FontData)
	HAS_INTERFACE(ff::IFontData)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<FontData>(L"fontData");
	});

FontData::FontData()
	: _bold(false)
	, _italic(false)
	, _createdFontFace(false)
	, _index(0)
{
}

FontData::~FontData()
{
}

bool FontData::GetBold() const
{
	return _bold;
}

bool FontData::GetItalic() const
{
	return _italic;
}

size_t FontData::GetIndex() const
{
	return _index;
}

ff::IData* FontData::GetData() const
{
	return _data;
}

IDWriteFontFaceX* FontData::GetFontFace()
{
	if (!_createdFontFace)
	{
		ff::LockMutex lock(_mutex);

		if (!_createdFontFace)
		{
			assertRetVal(_data, nullptr);

			ff::IGraphicFactoryDWrite* dwrite = ff::ProcessGlobals::Get()->GetGraphicFactory()->AsGraphicFactoryDWrite();
			assertRetVal(dwrite, nullptr);

			IDWriteFactoryX* factory = dwrite->GetWriteFactory();
			IDWriteInMemoryFontFileLoader* loader = dwrite->GetFontDataLoader();
			assertRetVal(factory && loader, nullptr);

			ff::ComPtr<IDWriteFontFile> fontFile;
			assertHrRetVal(loader->CreateInMemoryFontFileReference(factory, _data->GetMem(), (UINT32)_data->GetSize(), _data, &fontFile), nullptr);

			ff::ComPtr<IDWriteFontFaceReference> fontFaceRef;
			assertHrRetVal(factory->CreateFontFaceReference(fontFile, (UINT32)_index,
				(_bold ? DWRITE_FONT_SIMULATIONS_BOLD : DWRITE_FONT_SIMULATIONS_NONE) |
				(_italic ? DWRITE_FONT_SIMULATIONS_OBLIQUE : DWRITE_FONT_SIMULATIONS_NONE),
				&fontFaceRef), nullptr);

			ff::ComPtr<IDWriteFontFace3> fontFace;
			assertHrRetVal(fontFaceRef->CreateFontFace(&fontFace), nullptr);
			assertRetVal(_fontFace.QueryFrom(fontFace), nullptr);

			_createdFontFace = true;
		}
	}

	return _fontFace;
}

bool FontData::LoadFromSource(const ff::Dict& dict)
{
	_bold = dict.Get<ff::BoolValue>(PROP_BOLD);
	_italic = dict.Get<ff::BoolValue>(PROP_ITALIC);
	_index = dict.Get<ff::SizeValue>(PROP_INDEX);

	ff::String path = dict.Get<ff::StringValue>(PROP_FILE);
	assertRetVal(ff::ReadWholeFile(path, &_data), false);

	return true;
}

bool FontData::LoadFromCache(const ff::Dict& dict)
{
	_bold = dict.Get<ff::BoolValue>(PROP_BOLD);
	_italic = dict.Get<ff::BoolValue>(PROP_ITALIC);
	_index = dict.Get<ff::SizeValue>(PROP_INDEX);
	_data = dict.Get<ff::DataValue>(PROP_DATA);

	return true;
}

bool FontData::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::BoolValue>(PROP_BOLD, _bold);
	dict.Set<ff::BoolValue>(PROP_ITALIC, _italic);
	dict.Set<ff::SizeValue>(PROP_INDEX, _index);
	dict.Set<ff::DataValue>(PROP_DATA, _data);

	return true;
}
