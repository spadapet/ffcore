#pragma once

namespace ff
{
	class IRenderDepth;
	class IRenderTarget;
	class IRenderer;
	class IRendererActive;
	class IRendererActive11;
	class ISprite;
	class RendererActive;
	enum class RendererOptions;
	struct PixelTransform;

	class PixelRendererActive
	{
	public:
		UTIL_API PixelRendererActive(RendererActive& render);

		UTIL_API static IRendererActive* BeginRender(IRenderer* render, IRenderTarget* target, IRenderDepth* depth, RectFixedInt viewRect, RectFixedInt worldRect, RendererOptions options = (RendererOptions)0);
		UTIL_API IRendererActive* GetRenderer() const;
		UTIL_API IRendererActive11* GetRenderer11() const;

		UTIL_API void DrawSprite(ISprite* sprite, const PixelTransform& pos) const;

		UTIL_API void DrawLineStrip(const PointFixedInt* points, size_t count, const DirectX::XMFLOAT4& color, FixedInt thickness) const;
		UTIL_API void DrawLine(PointFixedInt start, PointFixedInt end, const DirectX::XMFLOAT4& color, FixedInt thickness) const;
		UTIL_API void DrawFilledRectangle(RectFixedInt rect, const DirectX::XMFLOAT4& color) const;
		UTIL_API void DrawFilledCircle(PointFixedInt center, FixedInt radius, const DirectX::XMFLOAT4& color) const;
		UTIL_API void DrawOutlineRectangle(RectFixedInt rect, const DirectX::XMFLOAT4& color, FixedInt thickness) const;
		UTIL_API void DrawOutlineCircle(PointFixedInt center, FixedInt radius, const DirectX::XMFLOAT4& color, FixedInt thickness) const;

		UTIL_API void DrawPaletteLineStrip(const PointFixedInt* points, size_t count, int color, FixedInt thickness) const;
		UTIL_API void DrawPaletteLine(PointFixedInt start, PointFixedInt end, int color, FixedInt thickness) const;
		UTIL_API void DrawPaletteFilledRectangle(RectFixedInt rect, int color) const;
		UTIL_API void DrawPaletteFilledCircle(PointFixedInt center, FixedInt radius, int color) const;
		UTIL_API void DrawPaletteOutlineRectangle(RectFixedInt rect, int color, FixedInt thickness) const;
		UTIL_API void DrawPaletteOutlineCircle(PointFixedInt center, FixedInt radius, int color, FixedInt thickness) const;

		UTIL_API void PushMatrix(const PixelTransform& pos) const;
		UTIL_API void PopMatrix() const;

	private:
		IRendererActive* _render;
	};
}
