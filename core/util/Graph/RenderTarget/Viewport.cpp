#include "pch.h"
#include "Graph/RenderTarget/Viewport.h"
#include "Graph/RenderTarget/RenderTarget.h"

ff::Viewport::Viewport()
	: _aspect(1920, 1080)
	, _padding(0, 0, 0, 0)
	, _viewport(0, 0, 0, 0)
	, _targetSize(0, 0)
{
}

ff::Viewport::Viewport(PointFloat aspect, RectFloat padding)
	: _aspect(aspect)
	, _padding(padding)
	, _viewport(0, 0, 0, 0)
	, _targetSize(0, 0)
{
}

ff::Viewport::~Viewport()
{
}

void ff::Viewport::SetAutoSize(PointFloat aspect, RectFloat padding)
{
	_aspect = aspect;
	_padding = padding;
}

ff::RectFloat ff::Viewport::GetView(IRenderTarget* target)
{
	PointFloat targetSize = target ? target->GetRotatedSize().ToType<float>() : _targetSize;
	if (targetSize != _targetSize)
	{
		RectFloat safeArea(targetSize);
		if (safeArea.Width() > _padding.left + _padding.right &&
			safeArea.Height() > _padding.top + _padding.bottom)
		{
			// Adjust for padding
			safeArea = safeArea.Deflate(_padding);
		}

		_viewport = (_aspect.x && _aspect.y) ? RectFloat(_aspect) : safeArea;
		_viewport = _viewport.ScaleToFit(safeArea);
		_viewport = _viewport.CenterWithin(safeArea);

		_viewport.left = std::floor(_viewport.left);
		_viewport.top = std::floor(_viewport.top);
		_viewport.right = std::floor(_viewport.right);
		_viewport.bottom = std::floor(_viewport.bottom);

		_targetSize = targetSize;
	}

	return _viewport;
}
