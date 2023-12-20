#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Graph/RenderTarget/RenderTargetSwapChain.h"
#include "UI/XamlGlobalState.h"
#include "UI/XamlView.h"
#include "UI/XamlViewState.h"

ff::XamlViewState::XamlViewState(std::shared_ptr<XamlView> view, ff::IRenderTarget* target)
	: _view(view)
	, _target(target)
{
	ff::AppGlobals* appGlobals = _view->GetGlobals()->GetAppGlobals();

	_view->SetSize(target);

	ff::ComPtr<ff::IRenderTargetSwapChain> swapChainTarget;
	if (swapChainTarget.QueryFrom(_target))
	{
		_sizeChangedCookie = swapChainTarget->SizeChanged().Add([this](ff::PointInt size, double scale, int rotate)
			{
				_view->SetSize(size, scale, rotate);
			});
	}
}

ff::XamlViewState::~XamlViewState()
{
	if (_sizeChangedCookie)
	{
		ff::ComPtr<ff::IRenderTargetSwapChain> swapChainTarget;
		if (swapChainTarget.QueryFrom(_target))
		{
			swapChainTarget->SizeChanged().Remove(_sizeChangedCookie);
		}
	}
}

ff::XamlView* ff::XamlViewState::GetView() const
{
	return _view.get();
}

std::shared_ptr<ff::State> ff::XamlViewState::Advance(ff::AppGlobals* globals)
{
	_view->Advance();
	return nullptr;
}

void ff::XamlViewState::OnFrameRendering(ff::AppGlobals* globals, ff::AdvanceType type)
{
	_view->PreRender();
}

void ff::XamlViewState::Render(ff::AppGlobals* globals, ff::IRenderTarget* target, ff::IRenderDepth* depth)
{
	_view->Render(target, depth);
}