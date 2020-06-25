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

	class __declspec(uuid("b4356d2d-1b85-400c-b3d6-cbff44352305")) __declspec(novtable)
		ISpriteFont : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual PointFloat DrawText(IRendererActive* render, StringRef text, const Transform& transform, const DirectX::XMFLOAT4& outlineColor = ff::GetColorNone()) = 0;
		virtual PointFloat MeasureText(StringRef text, PointFloat scale) = 0;
		virtual float GetLineSpacing() = 0;
	};
}
