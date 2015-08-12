#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class IRenderTarget11;
	class IRenderTarget12;
	class ITexture;

	class __declspec(uuid("fcfc35d1-be2d-4630-b09d-58100ab1bf7c")) __declspec(novtable)
		IRenderTarget : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual PointInt GetBufferSize() const = 0;
		virtual PointInt GetRotatedSize() const = 0;
		virtual int GetRotatedDegrees() const = 0;
		virtual double GetDpiScale() const = 0;
		virtual void Clear(const DirectX::XMFLOAT4* pColor = nullptr) = 0;

		virtual IRenderTarget11* AsRenderTarget11() = 0;
	};

	class IRenderTarget11
	{
	public:
		virtual ID3D11Texture2D* GetTexture() = 0;
		virtual ID3D11RenderTargetView* GetTarget() = 0;
	};
}
