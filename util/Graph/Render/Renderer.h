#pragma once

namespace ff
{
	class IGraphDevice;
	class IRendererActive;
	class IRenderDepth;
	class IRenderTarget;

	enum class RendererOptions
	{
		None = 0x00,
		PreMultipliedAlpha = 0x01,
	};

	class IRenderer
	{
	public:
		virtual ~IRenderer() { }
		virtual bool IsValid() const = 0;
		virtual IRendererActive* BeginRender(IRenderTarget* target, IRenderDepth* depth, RectFloat viewRect, RectFloat worldRect, RendererOptions options = RendererOptions::None) = 0;
	};
}
