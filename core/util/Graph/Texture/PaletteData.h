#pragma once

namespace ff
{
	class IData;

	const size_t PALETTE_SIZE = 256;

	class __declspec(uuid("ddb6b3f3-f4f2-4223-beab-64bdb67d284f")) __declspec(novtable)
		IPaletteData : public IUnknown
	{
	public:
		virtual IData* GetColors() const = 0; // 256 R8G8B8A8_UNORM colors
	};
}
