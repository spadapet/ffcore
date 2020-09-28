#pragma once

namespace ff
{
	enum class SpriteType
	{
		Unknown = 0x00,
		Opaque = 0x01,
		Transparent = 0x02,
		Palette = 0x10,

		OpaquePalette = Opaque | Palette,
	};
}
