#pragma once

namespace ff
{
	class IGraphDevice;
	class IRendererActive;
	class IRenderDepth;
	class IRenderTarget;

	class IRenderer
	{
	public:
		virtual ~IRenderer() { }
		virtual bool IsValid() const = 0;
		virtual IRendererActive* BeginRender(IRenderTarget* target, IRenderDepth* depth, RectFloat viewRect, RectFloat worldRect) = 0;
	};
}
