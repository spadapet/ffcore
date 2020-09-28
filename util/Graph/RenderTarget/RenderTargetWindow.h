#pragma once

#include "Graph/RenderTarget/RenderTargetSwapChain.h"
#include "Windows/WinUtil.h"

namespace ff
{
	class __declspec(uuid("429e3eb7-2116-4a37-aa65-af17b30f1761")) __declspec(novtable)
		IRenderTargetWindow : public IRenderTargetSwapChain
	{
	public:
		virtual bool CanSetFullScreen() const = 0;
		virtual bool IsFullScreen() = 0;
		virtual bool SetFullScreen(bool fullScreen) = 0;
	};
}
