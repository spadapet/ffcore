#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class IRendererActive;
	struct Transform;

	enum class SpriteFontControl : wchar_t
	{
		NoOp = 0xE000,
		SetTextColor, // R, G, B, A
		SetOutlineColor, // R, G, B, A
		SetTextPaletteColor, // Index
		SetOutlinePaletteColor, // Index

		AfterLast
	};

	enum class SpriteFontOptions
	{
		None,
		NoOutline = 0x01, // Only draw text
		NoText = 0x02, // Only draw outline
		NoControl = 0x04, // ignore any SpriteFontControl chars
	};

	class __declspec(uuid("b4356d2d-1b85-400c-b3d6-cbff44352305")) __declspec(novtable)
		ISpriteFont : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual PointFloat DrawText(
			IRendererActive* render,
			StringRef text,
			const Transform& transform,
			const DirectX::XMFLOAT4& outlineColor = ff::GetColorNone(),
			SpriteFontOptions options = SpriteFontOptions::None) = 0;

		virtual PointFloat MeasureText(StringRef text, PointFloat scale) = 0;
		virtual float GetLineSpacing() = 0;
	};
}
