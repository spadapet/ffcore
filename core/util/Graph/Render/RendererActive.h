#pragma once

namespace ff
{
	class GraphContext11;
	class IRendererActive11;
	class IPalette;
	class ISprite;
	class MatrixStack;
	struct Transform;

	typedef std::function<bool(GraphContext11& context, const std::type_info& vertexType, bool opaqueOnly)> CustomRenderContextFunc11;

	class IRendererActive
	{
	public:
		virtual void EndRender() = 0;
		virtual MatrixStack& GetWorldMatrixStack() = 0;
		virtual IRendererActive11* AsRendererActive11() = 0;

		virtual void DrawSprite(ISprite* sprite, const Transform& transform) = 0;
		virtual void DrawFont(ISprite* sprite, const Transform& transform) = 0;
		virtual void DrawLineStrip(const PointFloat* points, const DirectX::XMFLOAT4* colors, size_t count, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawLineStrip(const PointFloat* points, size_t count, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawLine(PointFloat start, PointFloat end, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawFilledRectangle(RectFloat rect, const DirectX::XMFLOAT4* colors) = 0;
		virtual void DrawFilledRectangle(RectFloat rect, const DirectX::XMFLOAT4& color) = 0;
		virtual void DrawFilledTriangles(const PointFloat* points, const DirectX::XMFLOAT4* colors, size_t count) = 0;
		virtual void DrawFilledCircle(PointFloat center, float radius, const DirectX::XMFLOAT4& color) = 0;
		virtual void DrawFilledCircle(PointFloat center, float radius, const DirectX::XMFLOAT4& insideColor, const DirectX::XMFLOAT4& outsideColor) = 0;
		virtual void DrawOutlineRectangle(RectFloat rect, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawOutlineCircle(PointFloat center, float radius, const DirectX::XMFLOAT4& color, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawOutlineCircle(PointFloat center, float radius, const DirectX::XMFLOAT4& insideColor, const DirectX::XMFLOAT4& outsideColor, float thickness, bool pixelThickness = false) = 0;

		virtual void DrawPaletteFont(ISprite* sprite, PointFloat pos, PointFloat scale, int color) = 0;
		virtual void DrawPaletteLineStrip(const PointFloat* points, const int* colors, size_t count, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawPaletteLineStrip(const PointFloat* points, size_t count, int color, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawPaletteLine(PointFloat start, PointFloat end, int color, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawPaletteFilledRectangle(RectFloat rect, const int* colors) = 0;
		virtual void DrawPaletteFilledRectangle(RectFloat rect, int color) = 0;
		virtual void DrawPaletteFilledTriangles(const PointFloat* points, const int* colors, size_t count) = 0;
		virtual void DrawPaletteFilledCircle(PointFloat center, float radius, int color) = 0;
		virtual void DrawPaletteFilledCircle(PointFloat center, float radius, int insideColor, int outsideColor) = 0;
		virtual void DrawPaletteOutlineRectangle(RectFloat rect, int color, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawPaletteOutlineCircle(PointFloat center, float radius, int color, float thickness, bool pixelThickness = false) = 0;
		virtual void DrawPaletteOutlineCircle(PointFloat center, float radius, int insideColor, int outsideColor, float thickness, bool pixelThickness = false) = 0;

		virtual void PushPalette(IPalette* palette) = 0;
		virtual void PopPalette() = 0;
		virtual void PushNoOverlap() = 0;
		virtual void PopNoOverlap() = 0;
		virtual void PushOpaque() = 0;
		virtual void PopOpaque() = 0;
		virtual void NudgeDepth() = 0;
	};

	class IRendererActive11
	{
	public:
		virtual void PushCustomContext(CustomRenderContextFunc11&& func) = 0;
		virtual void PopCustomContext() = 0;
		virtual void PushTextureSampler(D3D11_FILTER filter) = 0;
		virtual void PopTextureSampler() = 0;
	};

	class RendererActive
	{
	public:
		UTIL_API RendererActive();
		UTIL_API RendererActive(ff::IRendererActive* render);
		UTIL_API RendererActive(RendererActive&& rhs);
		UTIL_API ~RendererActive();
		UTIL_API RendererActive& operator=(ff::IRendererActive* render);

		UTIL_API void EndRender();

		UTIL_API operator ff::IRendererActive* () const;
		UTIL_API ff::IRendererActive& operator*() const;
		UTIL_API ff::IRendererActive* operator->() const;
		UTIL_API bool operator!() const;

	private:
		ff::IRendererActive* _render;
	};
}
