#include "pch.h"
#include "Data/Data.h"
#include "Dict/Dict.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/Texture/PaletteData.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Value/Values.h"
#include "Windows/FileUtil.h"

static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FILE(L"file");

class __declspec(uuid("d1176b8c-4ebb-40a6-9f6a-f92044187a37"))
	PaletteData
	: public ff::ComBase
	, public ff::IPaletteData
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(PaletteData);

	bool Init(ff::IData* colors);

	// IPaletteData functions
	virtual ff::IData* GetColors() const override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	ff::ComPtr<ff::IData> _colors;
};

BEGIN_INTERFACES(PaletteData)
	HAS_INTERFACE(ff::IPaletteData)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<PaletteData>(L"palette");
	});

bool ff::CreatePaletteData(IData* colors, IPaletteData** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ComPtr<PaletteData, IPaletteData> newObj;
	assertHrRetVal(ff::ComAllocator<PaletteData>::CreateInstance(&newObj), false);
	assertRetVal(newObj->Init(colors), false);

	*obj = newObj.Detach();
	return true;
}

PaletteData::PaletteData()
{
}

PaletteData::~PaletteData()
{
}

bool PaletteData::Init(ff::IData* colors)
{
	assertRetVal(colors, false);
	_colors = colors;
	return true;
}

ff::IData* PaletteData::GetColors() const
{
	return _colors;
}

bool PaletteData::LoadFromSource(const ff::Dict& dict)
{
	ff::String path = dict.Get<ff::StringValue>(PROP_FILE);

	ff::ComPtr<ff::IData> fileData;
	assertRetVal(ff::ReadWholeFileMemMapped(path, &fileData) && fileData->GetSize() && fileData->GetSize() % 3 == 0, false);

	ff::Vector<BYTE> colors;
	{
		colors.Resize(ff::PALETTE_SIZE * 4);
		::ZeroMemory(colors.Data(), colors.ByteSize());

		BYTE* dest = colors.Data();
		for (const BYTE* start = fileData->GetMem(), *end = start + fileData->GetSize(), *cur = start;
			cur < end && cur - start < ff::PALETTE_SIZE * 3; cur += 3, dest += 4)
		{
			dest[0] = cur[0]; // R
			dest[1] = cur[1]; // G
			dest[2] = cur[2]; // B
			dest[3] = 0xFF; // A
		}
	}

	_colors = ff::CreateDataVector(std::move(colors));

	return true;
}

bool PaletteData::LoadFromCache(const ff::Dict& dict)
{
	_colors = dict.Get<ff::DataValue>(PROP_DATA);

	return true;
}

bool PaletteData::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::DataValue>(PROP_DATA, _colors);

	return true;
}
