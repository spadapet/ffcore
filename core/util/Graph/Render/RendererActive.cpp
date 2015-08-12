#include "pch.h"
#include "Graph/Render/RendererActive.h"

ff::RendererActive::RendererActive()
	: _render(nullptr)
{
}

ff::RendererActive::RendererActive(ff::IRendererActive* render)
	: _render(render)
{
}

ff::RendererActive::RendererActive(RendererActive&& rhs)
	: _render(rhs._render)
{
	rhs._render = nullptr;
}

ff::RendererActive::~RendererActive()
{
	EndRender();
}

ff::RendererActive& ff::RendererActive::operator=(ff::IRendererActive* render)
{
	if (_render != render)
	{
		EndRender();
		_render = render;
	}

	return *this;
}

void ff::RendererActive::EndRender()
{
	if (_render)
	{
		_render->EndRender();
		_render = nullptr;
	}
}

ff::RendererActive::operator ff::IRendererActive* () const
{
	return _render;
}

ff::IRendererActive& ff::RendererActive::operator*() const
{
	return *_render;
}

ff::IRendererActive* ff::RendererActive::operator->() const
{
	return _render;
}

bool ff::RendererActive::operator!() const
{
	return _render == nullptr;
}
