#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class IGraphDevice;
	class IPalette;
	class ITexture;

	const size_t PALETTE_SIZE = 256;
	const size_t PALETTE_ROW_BYTES = 256 * 4;

	class __declspec(uuid("ddb6b3f3-f4f2-4223-beab-64bdb67d284f")) __declspec(novtable)
		IPaletteData : public IUnknown, public ff::IGraphDeviceChild
	{
	public:
		virtual size_t GetRowCount() const = 0;
		virtual hash_t GetRowHash(size_t index) const = 0;
		virtual ITexture* GetTexture() = 0;

		virtual ComPtr<IPalette> CreatePalette(float cyclesPerSecond = 0.0f) = 0;
	};

	bool CreatePaletteData(ff::IGraphDevice* device, DirectX::ScratchImage&& paletteScratch, ff::IPaletteData** obj);
}
