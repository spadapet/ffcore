#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class IRendererActive;
	struct Transform;

	struct FontColorChange
	{
		size_t _index;
		DirectX::XMFLOAT4 _color;
	};

	struct FontPaletteChange
	{
		size_t _index;
		int _color;
	};

	class __declspec(uuid("b4356d2d-1b85-400c-b3d6-cbff44352305")) __declspec(novtable)
		ISpriteFont : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual PointFloat DrawText(
			IRendererActive* render,
			StringRef text,
			size_t charCount,
			const Transform& transform,
			const FontColorChange* colorChanges = nullptr,
			size_t colorCount = 0,
			const FontColorChange* outlineColorChanges = nullptr,
			size_t outlineColorCount = 0) = 0;

		virtual PointFloat DrawPaletteText(
			IRendererActive* render,
			StringRef text,
			size_t charCount,
			const Transform& transform,
			const FontPaletteChange* colorChanges = nullptr,
			size_t colorCount = 0,
			const FontPaletteChange* outlineColorChanges = nullptr,
			size_t outlineColorCount = 0) = 0;

		virtual PointFloat MeasureText(StringRef text, size_t charCount, PointFloat scale) = 0;
		virtual float GetLineSpacing() = 0;
	};
}
