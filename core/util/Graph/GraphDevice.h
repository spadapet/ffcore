#pragma once

#include "Windows/WinUtil.h"

namespace DirectX
{
	class ScratchImage;
}

namespace ff
{
	class GraphContext11;
	class GraphStateCache11;
	class IData;
	class IGraphBuffer;
	class IGraphDevice11;
	class IGraphDevice12;
	class IGraphDeviceChild;
	class IGraphDeviceDxgi;
	class IGraphDeviceInternal;
	class IRenderDepth;
	class IRenderer;
	class IRenderTarget;
	class IRenderTargetSwapChain;
	class IRenderTargetWindow;
	class ITexture;
	enum class GraphBufferType;
	enum class SpriteType;
	enum class TextureFormat;

	class __declspec(uuid("1b26d121-cda5-4705-ae3d-4815b4a4115b")) __declspec(novtable)
		IGraphDevice : public IUnknown
	{
	public:
		virtual bool Reset() = 0;
		virtual bool ResetIfNeeded() = 0;
		virtual size_t ResetDrawCount() = 0;

		virtual IGraphDeviceDxgi* AsGraphDeviceDxgi() = 0;
		virtual IGraphDevice11* AsGraphDevice11() = 0;
		virtual IGraphDeviceInternal* AsGraphDeviceInternal() = 0;

		virtual ComPtr<IGraphBuffer> CreateBuffer(GraphBufferType type, size_t size, bool writable = true, ff::IData* initialData = nullptr) = 0;
		virtual ComPtr<ITexture> CreateTexture(StringRef path, TextureFormat format, size_t mips = 1) = 0;
		virtual ComPtr<ITexture> CreateTexture(PointInt size, TextureFormat format, size_t mips = 1, size_t count = 1, size_t samples = 1) = 0;
		virtual ComPtr<ITexture> CreateStagingTexture(PointInt size, TextureFormat format, bool readable, bool writable, size_t mips = 1, size_t count = 1, size_t samples = 1) = 0;
		virtual std::unique_ptr<IRenderer> CreateRenderer() = 0;
		virtual ComPtr<IRenderDepth> CreateRenderDepth(PointInt size = PointInt(1, 1), size_t samples = 1) = 0;
		virtual ComPtr<IRenderTarget> CreateRenderTargetTexture(ITexture* texture, size_t arrayStart = 0, size_t arrayCount = 1, size_t mipLevel = 0) = 0;
#if METRO_APP
		virtual ComPtr<IRenderTargetWindow> CreateRenderTargetWindow(Windows::UI::Xaml::Window^ hwnd) = 0;
		virtual ComPtr<IRenderTargetSwapChain> CreateRenderTargetSwapChain(Windows::UI::Xaml::Controls::SwapChainPanel^ panel) = 0;
#else
		virtual ComPtr<IRenderTargetWindow> CreateRenderTargetWindow(HWND hwnd) = 0;
#endif

		virtual void AddChild(IGraphDeviceChild* child) = 0;
		virtual void RemoveChild(IGraphDeviceChild* child) = 0;
	};

	class IGraphDeviceDxgi
	{
	public:
		virtual IDXGIDeviceX* GetDxgi() = 0;
		virtual IDXGIAdapterX* GetAdapter() = 0;
		virtual IDXGIFactoryX* GetDxgiFactory() = 0;
	};

	class IGraphDevice11
	{
	public:
		virtual ID3D11DeviceX* Get3d() = 0;
		virtual ID2D1DeviceX* Get2d() = 0;
		virtual ID3D11DeviceContextX* GetContext() = 0;

		virtual GraphContext11& GetStateContext() = 0;
		virtual GraphStateCache11& GetStateCache() = 0;
	};

	class IGraphDeviceInternal
	{
	public:
		virtual ComPtr<ITexture> CreateTexture(DirectX::ScratchImage&& data, SpriteType spriteType) = 0;
	};
}
