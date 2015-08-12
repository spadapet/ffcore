#pragma once

namespace ff
{
	class ISpriteList;
	enum class TextureFormat;

	UTIL_API bool OptimizeSprites(ISpriteList* originalSprites, TextureFormat format, size_t mipMapLevels, ISpriteList** outSprites);
	UTIL_API bool CreateOutlineSprites(ISpriteList* originalSprites, TextureFormat format, size_t mipMapLevels, ISpriteList** outSprites);
}
