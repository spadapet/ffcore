#include "pch.h"
#include "UI/Internal/XamlView11.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/State/GraphContext11.h"
#include "UI/XamlGlobalState.h"

ff::XamlView11::XamlView11(XamlGlobalState* globals, Noesis::FrameworkElement* content, bool perPixelAntiAlias, bool subPixelRendering)
	: XamlView(globals, content, perPixelAntiAlias, subPixelRendering)
{
}

ff::XamlView11::~XamlView11()
{
}

void ff::XamlView11::RenderBegin(ff::IRenderTarget* target, ff::IRenderDepth* depth, const ff::RectFloat* viewRect)
{
	assertRet(target && depth && depth->SetSize(target->GetBufferSize()));

	ff::PointFloat targetSize = target->GetBufferSize().ToType<float>();
	ID3D11RenderTargetView* targetView = target->AsRenderTarget11()->GetTarget();
	ID3D11DepthStencilView* depthView = depth->AsRenderDepth11()->GetView();
	assertRet(targetView && depthView);

	D3D11_VIEWPORT viewport{};
	viewport.MaxDepth = 1;

	if (viewRect)
	{
		viewport.TopLeftX = viewRect->left;
		viewport.TopLeftY = viewRect->top;
		viewport.Width = viewRect->Width();
		viewport.Height = viewRect->Height();
	}
	else
	{
		viewport.Width = targetSize.x;
		viewport.Height = targetSize.y;
	}

	ff::GraphContext11& context = target->GetDevice()->AsGraphDevice11()->GetStateContext();
	context.SetTargets(&targetView, 1, depthView);
	context.SetViewports(&viewport, 1);
}

void ff::XamlView11::RenderEnd(ff::IRenderTarget* target, ff::IRenderDepth* depth, const ff::RectFloat* viewRect)
{
}
