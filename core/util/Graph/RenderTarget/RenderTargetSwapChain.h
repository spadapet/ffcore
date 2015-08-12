#pragma once

#include "Graph/RenderTarget/RenderTarget.h"
#include "Types/Event.h"

namespace ff
{
	class __declspec(uuid("6eac1f1b-0101-4532-8787-bb38e370e137")) __declspec(novtable)
		IRenderTargetSwapChain : public IRenderTarget
	{
	public:
		virtual bool Present(bool vsync) = 0;
		virtual bool SetSize(ff::PointInt pixelSize, double dpiScale, DXGI_MODE_ROTATION nativeOrientation, DXGI_MODE_ROTATION currentOrientation) = 0;
		virtual ff::Event<ff::PointInt, double, int>& SizeChanged() = 0; // bufferPixelSize, dpiScale, rotation
	};
}
