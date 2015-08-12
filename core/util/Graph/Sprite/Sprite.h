#pragma once

#include "Resource/SharedResourceValue.h"

namespace ff
{
	class ITextureView;
	class ISpriteList;

	enum class SpriteType
	{
		Unknown,
		Opaque,
		Transparent,
	};

	struct SpriteData
	{
		UTIL_API RectInt GetTextureRect() const;
		UTIL_API RectFloat GetTextureRectF() const;
		UTIL_API PointFloat GetScale() const;
		UTIL_API PointFloat GetHandle() const;

		String _name;
		ITextureView* _textureView;
		RectFloat _textureUV;
		RectFloat _worldRect;
		SpriteType _type;
	};

	class __declspec(uuid("960fc0cc-a692-4a3e-9e37-59e7ca330aac")) __declspec(novtable)
		ISprite : public IUnknown
	{
	public:
		virtual const SpriteData& GetSpriteData() = 0;
	};

	class __declspec(uuid("2b784b98-e637-4a35-b5db-bb2233fdd367")) __declspec(novtable)
		ISpriteResource : public ISprite
	{
	public:
		virtual SharedResourceValue GetSourceResource() = 0;
	};

	UTIL_API bool CreateSprite(const SpriteData& data, ISprite** ppSprite);
	UTIL_API bool CreateSprite(ITextureView* textureView, StringRef name, RectFloat rect, PointFloat handle, PointFloat scale, SpriteType type, ISprite** ppSprite);

	// For when you have a delayed resource, but need an ISprite right now
	UTIL_API bool CreateSpriteResource(SharedResourceValue spriteOrListRes, ISprite** obj);
	UTIL_API bool CreateSpriteResource(SharedResourceValue spriteOrListRes, StringRef name, ISprite** obj);
	UTIL_API bool CreateSpriteResource(SharedResourceValue spriteOrListRes, size_t index, ISprite** obj);
}
