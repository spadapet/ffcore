#include "pch.h"
#include "Graph/State/GraphContext11.h"
#include "Graph/State/GraphFixedState11.h"

ff::GraphFixedState11::GraphFixedState11()
	: _blendFactor(ff::GetColorWhite())
	, _sampleMask(0xFFFFFFFF)
	, _stencil(0)
{
}

ff::GraphFixedState11::GraphFixedState11(GraphFixedState11&& rhs)
	: _blendFactor(rhs._blendFactor)
	, _sampleMask(rhs._sampleMask)
	, _stencil(rhs._stencil)
	, _raster(std::move(rhs._raster))
	, _blend(std::move(rhs._blend))
	, _depth(std::move(rhs._depth))
	, _disabledDepth(std::move(rhs._disabledDepth))
{
}

ff::GraphFixedState11::GraphFixedState11(const GraphFixedState11& rhs)
{
	*this = rhs;
}

ff::GraphFixedState11& ff::GraphFixedState11::operator=(const GraphFixedState11& rhs)
{
	if (this != &rhs)
	{
		_blendFactor = rhs._blendFactor;
		_sampleMask = rhs._sampleMask;
		_stencil = rhs._stencil;

		_raster = rhs._raster;
		_blend = rhs._blend;
		_depth = rhs._depth;
		_disabledDepth = rhs._disabledDepth;
	}

	return *this;
}

void ff::GraphFixedState11::Apply(GraphContext11& context) const
{
	if (_raster)
	{
		context.SetRaster(_raster);
	}

	if (_blend)
	{
		context.SetBlend(_blend, _blendFactor, _sampleMask);
	}

	if (_disabledDepth && context.GetDepthView() == nullptr)
	{
		context.SetDepth(_disabledDepth, _stencil);
	}
	else if (_depth)
	{
		context.SetDepth(_depth, _stencil);
	}
}
