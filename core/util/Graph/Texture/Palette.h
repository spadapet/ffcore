#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class IPaletteData;
	class ITextureView;

	class __declspec(uuid("2e0118f8-f59c-4037-b88e-0fd958f8155b")) __declspec(novtable)
		IPalette : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual void Advance() = 0;
		virtual ITextureView* GetTextureView() = 0;
	};

	bool CreatePalette(IGraphDevice* device, IPaletteData* data, IPalette** obj);
}
