#pragma once

namespace ff
{
	enum class TextureFormat
	{
		Unknown,
		A8,
		R1,
		R8,
		R8_UINT,
		R8G8,

		RGBA32,
		BGRA32,
		BC1,
		BC2,
		BC3,

		RGBA32_SRGB,
		BGRA32_SRGB,
		BC1_SRGB,
		BC2_SRGB,
		BC3_SRGB,
	};
}
