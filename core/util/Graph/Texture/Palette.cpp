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

	bool Init(ff::IPaletteData* data, float cyclesPerSecond);

	// IPalette
	virtual void Advance() override;
	virtual size_t GetCurrentRow() const override;
	virtual ff::IPaletteData* GetData() override;

private:
	ff::ComPtr<ff::IPaletteData> _data;
	float _cps;
	float _advances;
	size_t _row;
};

BEGIN_INTERFACES(Palette)
	HAS_INTERFACE(ff::IPalette)
END_INTERFACES()

bool CreatePalette(ff::IPaletteData* data, float cyclesPerSecond, ff::IPalette** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ComPtr<Palette, ff::IPalette> newObj;
	assertHrRetVal(ff::ComAllocator<Palette>::CreateInstance(&newObj), false);
	assertRetVal(newObj->Init(data, cyclesPerSecond), false);

	*obj = newObj.Detach();
	return true;
}

Palette::Palette()
	: _cps(0)
	, _advances(0)
	, _row(0)
{
}

Palette::~Palette()
{
}

bool Palette::Init(ff::IPaletteData* data, float cyclesPerSecond)
{
	assertRetVal(data, false);

	_data = data;
	_cps = cyclesPerSecond;

	return true;
}

void Palette::Advance()
{
	size_t count = _data->GetRowCount();
	_row = (size_t)(++_advances * _cps * count / 60.0f) % count;
}

ff::IPaletteData* Palette::GetData()
{
	return _data;
}

size_t Palette::GetCurrentRow() const
{
	return _row;
}
