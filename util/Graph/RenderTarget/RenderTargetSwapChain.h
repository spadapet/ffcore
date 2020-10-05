#pragma once

#include "Graph/RenderTarget/RenderTarget.h"

namespace ff
{
	struct SwapChainSize;

	class __declspec(uuid("6eac1f1b-0101-4532-8787-bb38e370e137")) __declspec(novtable)
		IRenderTargetSwapChain : public IRenderTarget
	{
	public:
		virtual bool Present(bool vsync) = 0;
		virtual bool SetSize(const ff::SwapChainSize& size) = 0;
		virtual entt::sink<void(ff::PointInt, double, int)> SizeChangedSink() = 0; // bufferPixelSize, dpiScale, rotation
	};
}
