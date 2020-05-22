#pragma once

#include "UI/XamlView.h"

namespace ff
{
	class XamlView11 : public XamlView
	{
	public:
		XamlView11(XamlGlobalState* globals, Noesis::FrameworkElement* content, ff::IRenderTarget* target, bool perPixelAntiAlias, bool subPixelRendering);
		virtual ~XamlView11();

	protected:
		virtual void RenderBegin(ff::IRenderTarget* target, ff::IRenderDepth* depth, const ff::RectFloat* viewRect) override;
		virtual void RenderEnd(ff::IRenderTarget* target, ff::IRenderDepth* depth, const ff::RectFloat* viewRect) override;
	};
}
