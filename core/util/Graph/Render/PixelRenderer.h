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
	struct PixelTransform;

	class PixelRendererActive
	{
	public:
		PixelRendererActive(RendererActive& render);

		static IRendererActive* BeginRender(IRenderer* render, IRenderTarget* target, IRenderDepth* depth, RectFixedInt viewRect, RectFixedInt worldRect);
		IRendererActive* GetRenderer() const;
		IRendererActive11* GetRenderer11() const;

		void DrawSprite(ISprite* sprite, const PixelTransform& pos) const;

		void DrawLineStrip(const PointFixedInt* points, size_t count, const DirectX::XMFLOAT4& color, FixedInt thickness) const;
		void DrawLine(PointFixedInt start, PointFixedInt end, const DirectX::XMFLOAT4& color, FixedInt thickness) const;
		void DrawFilledRectangle(RectFixedInt rect, const DirectX::XMFLOAT4& color) const;
		void DrawFilledCircle(PointFixedInt center, FixedInt radius, const DirectX::XMFLOAT4& color) const;
		void DrawOutlineRectangle(RectFixedInt rect, const DirectX::XMFLOAT4& color, FixedInt thickness) const;
		void DrawOutlineCircle(PointFixedInt center, FixedInt radius, const DirectX::XMFLOAT4& color, FixedInt thickness) const;

		void DrawPaletteLineStrip(const PointFixedInt* points, size_t count, int color, FixedInt thickness) const;
		void DrawPaletteLine(PointFixedInt start, PointFixedInt end, int color, FixedInt thickness) const;
		void DrawPaletteFilledRectangle(RectFixedInt rect, int color) const;
		void DrawPaletteFilledCircle(PointFixedInt center, FixedInt radius, int color) const;
		void DrawPaletteOutlineRectangle(RectFixedInt rect, int color, FixedInt thickness) const;
		void DrawPaletteOutlineCircle(PointFixedInt center, FixedInt radius, int color, FixedInt thickness) const;

		void PushMatrix(const PixelTransform& pos) const;
		void PopMatrix() const;

	private:
		IRendererActive* _render;
	};
}
