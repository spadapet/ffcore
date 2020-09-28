#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/State/GraphContext11.h"
#include "UI/XamlGlobalState.h"
#include "UI/XamlView.h"

ff::XamlView::XamlView(XamlGlobalState* globals, Noesis::FrameworkElement* content, bool perPixelAntiAlias, bool subPixelRendering)
	: _globals(globals)
	, _focused(false)
	, _enabled(true)
	, _blockedBelow(false)
	, _cursor(Noesis::Cursor_Arrow)
	, _viewGrid(Noesis::MakePtr<Noesis::Grid>())
	, _viewBox(Noesis::MakePtr<Noesis::Viewbox>())
	, _view(Noesis::GUI::CreateView(_viewBox))
	, _rotate(Noesis::MakePtr<Noesis::RotateTransform>())
	, _content(content)
	, _matrix(new DirectX::XMMATRIX[2])
	, _counter(0)
{
	_matrix[0] = DirectX::XMMatrixIdentity();
	_matrix[1] = DirectX::XMMatrixIdentity();

	_globals->RegisterView(this);

	_view->SetFlags((perPixelAntiAlias ? Noesis::RenderFlags_PPAA : 0) | (subPixelRendering ? Noesis::RenderFlags_LCD : 0));
	_view->GetRenderer()->Init(_globals->GetRenderDevice());
	_view->Deactivate();

	assert(!_content->GetParent());

	_viewGrid->SetLayoutTransform(_rotate);
	_viewGrid->GetChildren()->Add(_content);
	_viewBox->SetStretchDirection(Noesis::StretchDirection::StretchDirection_Both);
	_viewBox->SetStretch(Noesis::Stretch::Stretch_Fill);
	_viewBox->SetChild(_viewGrid);
}

ff::XamlView::~XamlView()
{
	Destroy();
	delete[] _matrix;
}

void ff::XamlView::Destroy()
{
	if (_view)
	{
		_view->GetRenderer()->Shutdown();
		_view = nullptr;
	}

	if (_globals)
	{
		_globals->UnregisterView(this);
		_globals = nullptr;
	}
}

Noesis::IView* ff::XamlView::GetView() const
{
	return _view;
}

ff::XamlGlobalState* ff::XamlView::GetGlobals() const
{
	assert(_globals);
	return _globals;
}

Noesis::FrameworkElement* ff::XamlView::GetContent() const
{
	return _content;
}

Noesis::Visual* ff::XamlView::HitTest(ff::PointFloat screenPos) const
{
	ff::PointFloat pos = ScreenToContent(screenPos);
	Noesis::HitTestResult ht = Noesis::VisualTreeHelper::HitTest(_content, Noesis::Point(pos.x, pos.y));
	return ht.visualHit;
}

Noesis::Cursor ff::XamlView::GetCursor() const
{
	return _cursor;
}

void ff::XamlView::SetCursor(Noesis::Cursor cursor)
{
	_cursor = cursor;
}

void ff::XamlView::SetSize(ff::IRenderTarget* target)
{
	if (target)
	{
		SetSize(target->GetBufferSize(), target->GetDpiScale(), target->GetRotatedDegrees());
	}
}

void ff::XamlView::SetSize(ff::PointInt pixelSize, double dpiScale, int rotate)
{
	assert(rotate >= 0 && rotate < 360 && rotate % 90 == 0);
	bool rotated = (rotate == 90 || rotate == 270);

	ff::PointType<float> dipSize = pixelSize.ToType<float>() / (float)dpiScale;

	_rotate->SetAngle((float)rotate);
	_viewGrid->SetWidth(rotated ? dipSize.y : dipSize.x);
	_viewGrid->SetHeight(rotated ? dipSize.x : dipSize.y);
	_view->SetSize((uint32_t)pixelSize.x, (uint32_t)pixelSize.y);
}

ff::PointFloat ff::XamlView::ScreenToContent(ff::PointFloat pos) const
{
	pos = ScreenToView(pos);
	Noesis::Point pos2 = GetContent()->PointFromScreen(Noesis::Point(pos.x, pos.y));
	return ff::PointFloat(pos2.x, pos2.y);
}

ff::PointFloat ff::XamlView::ContentToScreen(ff::PointFloat pos) const
{
	Noesis::Point viewPos = GetContent()->PointToScreen(Noesis::Point(pos.x, pos.y));
	return ViewToScreen(ff::PointFloat(viewPos.x, viewPos.y));
}

void ff::XamlView::SetViewToScreenTransform(ff::PointFloat pos, ff::PointFloat scale)
{
	SetViewToScreenTransform(DirectX::XMMatrixAffineTransformation2D(
		DirectX::XMVectorSet(scale.x, scale.y, 1, 1), // scale
		DirectX::XMVectorSet(0, 0, 0, 0), // rotation center
		0, // rotation
		DirectX::XMVectorSet(pos.x, pos.y, 0, 0))); // translation
}

void ff::XamlView::SetViewToScreenTransform(const DirectX::XMMATRIX& matrix)
{
	if (matrix != _matrix[0])
	{
		_matrix[0] = matrix;
		_matrix[1] = DirectX::XMMatrixInverse(nullptr, _matrix[0]);
	}
}

ff::PointFloat ff::XamlView::ScreenToView(ff::PointFloat pos) const
{
	DirectX::XMFLOAT2 viewPos;
	DirectX::XMStoreFloat2(&viewPos, DirectX::XMVector2Transform(DirectX::XMVectorSet(pos.x, pos.y, 0, 0), _matrix[1]));
	return ff::PointFloat(viewPos.x, viewPos.y);
}

ff::PointFloat ff::XamlView::ViewToScreen(ff::PointFloat pos) const
{
	DirectX::XMFLOAT2 screenPos;
	DirectX::XMStoreFloat2(&screenPos, DirectX::XMVector2Transform(DirectX::XMVectorSet(pos.x, pos.y, 0, 0), _matrix[0]));
	return ff::PointFloat(screenPos.x, screenPos.y);
}

void ff::XamlView::SetFocus(bool focus)
{
	if (_focused != focus)
	{
		if (focus)
		{
			_focused = true;
			_view->Activate();
		}
		else
		{
			_focused = false;
			_view->Deactivate();
		}

		_globals->OnFocusView(this, focus);
	}
}

bool ff::XamlView::IsFocused() const
{
	return _focused;
}

void ff::XamlView::SetEnabled(bool enabled)
{
	_enabled = enabled;
}

bool ff::XamlView::IsEnabled() const
{
	return _enabled;
}

void ff::XamlView::SetBlockInputBelow(bool block)
{
	_blockedBelow = block;
}

bool ff::XamlView::IsInputBelowBlocked() const
{
	return _blockedBelow;
}

void ff::XamlView::Advance()
{
	double time = _counter++ * ff::SECONDS_PER_ADVANCE_D;

	if (_view->Update(time))
	{
		_view->GetRenderer()->UpdateRenderTree();
	}
}

void ff::XamlView::PreRender()
{
	_view->GetRenderer()->RenderOffscreen();
}

void ff::XamlView::Render(ff::IRenderTarget* target, ff::IRenderDepth* depth, const ff::RectFloat* viewRect)
{
	_globals->OnRenderView(this);
	RenderBegin(target, depth, viewRect);
	_view->GetRenderer()->Render();
	RenderEnd(target, depth, viewRect);
}
