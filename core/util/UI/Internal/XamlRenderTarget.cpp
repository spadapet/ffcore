#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/Texture/Texture.h"
#include "UI/Internal/XamlRenderTarget.h"
#include "UI/Internal/XamlTexture.h"
#include "UI/XamlGlobalState.h"

ff::XamlRenderTarget::XamlRenderTarget(ff::IGraphDevice* graph, size_t width, size_t height, size_t samples, bool sRGB, ff::StringRef name)
	: _name(name)
	, _graph(graph)
{
	ff::PointInt size = ff::PointSize(width, height).ToType<int>();
	ff::TextureFormat format = sRGB ? ff::TextureFormat::RGBA32_SRGB : ff::TextureFormat::RGBA32;

	_msaaTexture = graph->CreateTexture(size, format, 1, 1, samples);
	_msaaTarget = graph->CreateRenderTargetTexture(_msaaTexture, 0, 1, 0);

	if (_msaaTexture->GetSampleCount() > 1)
	{
		_resolvedTexture = graph->CreateTexture(size, format, 1, 1, 1);
		_resolvedTarget = graph->CreateRenderTargetTexture(_resolvedTexture, 0, 1, 0);
	}
	else
	{
		_resolvedTexture = _msaaTexture;
		_resolvedTarget = _msaaTarget;
	}

	_resolvedTextureWrapper = Noesis::MakePtr<XamlTexture>(_resolvedTexture, name);
	_depth = graph->CreateRenderDepth(size, _msaaTexture->GetSampleCount());
}

ff::XamlRenderTarget::XamlRenderTarget(const XamlRenderTarget& rhs, ff::StringRef name)
	: _name(name)
	, _graph(rhs._graph)
	, _depth(rhs._depth)
{
	_resolvedTexture = _graph->CreateTexture(rhs._resolvedTexture->GetSize(), rhs._resolvedTexture->GetFormat(), 1, 1, 1);
	_resolvedTarget = _graph->CreateRenderTargetTexture(_resolvedTexture, 0, 1, 0);
	_resolvedTextureWrapper = Noesis::MakePtr<XamlTexture>(_resolvedTexture, name);

	if (rhs._msaaTexture->GetSampleCount() > 1)
	{
		_msaaTexture = rhs._msaaTexture;
		_msaaTarget = rhs._msaaTarget;
	}
	else
	{
		_msaaTexture = _resolvedTexture;
		_msaaTarget = _resolvedTarget;
	}
}

ff::XamlRenderTarget::~XamlRenderTarget()
{
}

ff::XamlRenderTarget* ff::XamlRenderTarget::Get(Noesis::RenderTarget* renderTarget)
{
	return static_cast<ff::XamlRenderTarget*>(renderTarget);
}

Noesis::Ptr<ff::XamlRenderTarget> ff::XamlRenderTarget::Clone(ff::StringRef name) const
{
	return *new XamlRenderTarget(*this, name);
}

ff::StringRef ff::XamlRenderTarget::GetName() const
{
	return _name;
}

ff::ITexture* ff::XamlRenderTarget::GetResolvedTexture() const
{
	return _resolvedTexture;
}

ff::ITexture* ff::XamlRenderTarget::GetMsaaTexture() const
{
	return _msaaTexture;
}

ff::IRenderTarget* ff::XamlRenderTarget::GetResolvedTarget() const
{
	return _resolvedTarget;
}

ff::IRenderTarget* ff::XamlRenderTarget::GetMsaaTarget() const
{
	return _msaaTarget;
}

ff::IRenderDepth* ff::XamlRenderTarget::GetDepth() const
{
	return _depth;
}

Noesis::Texture* ff::XamlRenderTarget::GetTexture()
{
	return _resolvedTextureWrapper;
}
