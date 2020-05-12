#pragma once

namespace ff
{
	class GraphContext11;
	class IRendererActive11;
	class ISprite;
	class ITexture;
	class MatrixStack;

	typedef std::function<bool(GraphContext11 & context, const std::type_info & vertexType, bool opaqueOnly)> CustomRenderContextFunc11;

	class IRendererActive
	{
	public:
		virtual void EndRender() = 0;
		virtual MatrixStack& GetWorldMatrixStack() = 0;
		virtual IRendererActive11* AsRendererActive11() = 0;

		virtual void DrawSprite(ISprite* sprite, PointFloat pos, PointFloat scale, const float rotate, const DirectX::XMFLOAT4& color) = 0;
		virtual void DrawMultiSprite(ISprite** sprites, const DirectX::XMFLOAT4* colors, size_t count, PointFloat pos, PointFloat scale, const float rotate) = 0;
		virtual void DrawFont(ISprite* sprite, PointFloat pos, PointFloat scale, const DirectX::XMFLOAT4& color) = 0;
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

		//virtual void PushPalette(ITexture* palette) = 0;
		//virtual void PopPalette() = 0;
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
