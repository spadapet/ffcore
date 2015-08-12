#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class IRenderDepth11;

	class __declspec(uuid("907bf3da-2a06-4bac-a929-69868dabd9f2")) __declspec(novtable)
		IRenderDepth : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual PointInt GetSize() const = 0;
		virtual bool SetSize(PointInt size) = 0;
		virtual void Clear(float depth = 0, BYTE stencil = 0) = 0;
		virtual void ClearDepth(float depth = 0) = 0;
		virtual void ClearStencil(BYTE stencil) = 0;
		virtual void Discard() = 0;

		virtual IRenderDepth11* AsRenderDepth11() = 0;
	};

	class IRenderDepth11
	{
	public:
		virtual ID3D11Texture2D* GetTexture() = 0;
		virtual ID3D11DepthStencilView* GetView() = 0;
	};
}
