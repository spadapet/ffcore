#pragma once

namespace ff
{
	class IPaletteData;
	class ITexture;

	class __declspec(uuid("2e0118f8-f59c-4037-b88e-0fd958f8155b")) __declspec(novtable)
		IPalette : public IUnknown
	{
	public:
		virtual void Advance() = 0;
		virtual size_t GetCurrentRow() const = 0;
		virtual IPaletteData* GetData() = 0;
	};
}
