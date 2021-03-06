#pragma once

#include "Graph/GraphDeviceChild.h"
#include "Graph/Sprite/SpriteType.h"

namespace ff
{
	class ISprite;
	class ITextureView;
	enum class SpriteType;

	// The source of sprites - they must all use the same texture
	class __declspec(uuid("9b4114d1-b515-4cfa-bafe-c27113cd4e75")) __declspec(novtable)
		ISpriteList : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual ISprite* Add(ITextureView* texture, StringRef name, RectFloat rect, PointFloat handle, PointFloat scale = PointFloat::Ones(), SpriteType type = SpriteType::Unknown) = 0;
		virtual ISprite* Add(ISprite* sprite) = 0;
		virtual bool Add(ISpriteList* list) = 0;

		virtual size_t GetCount() = 0;
		virtual ISprite* Get(size_t index) = 0;
		virtual ISprite* Get(StringRef name) = 0;
		virtual StringRef GetName(size_t index) = 0;
		virtual size_t GetIndex(StringRef name) = 0;
		virtual bool Remove(ISprite* sprite) = 0;
		virtual bool Remove(size_t index) = 0;
	};

	UTIL_API bool CreateSpriteList(IGraphDevice* device, ISpriteList** list);
}
