#pragma once

namespace ff
{
	class IGraphDevice;
	class IGraphicFactory2d;
	class IGraphicFactoryDWrite;
	class IGraphicFactoryDxgi;

	class __declspec(uuid("d29743fc-f64f-4f7c-b011-1bd6e2a5a60e")) __declspec(novtable)
		IGraphicFactory : public IUnknown
	{
	public:
		virtual ComPtr<IGraphDevice> CreateDevice() = 0;
		virtual size_t GetDeviceCount() const = 0;
		virtual IGraphDevice* GetDevice(size_t nIndex) const = 0;

		virtual void AddChild(IGraphDevice* child) = 0;
		virtual void RemoveChild(IGraphDevice* child) = 0;

		virtual IGraphicFactoryDxgi* AsGraphicFactoryDxgi() = 0;
		virtual IGraphicFactory2d* AsGraphicFactory2d() = 0;
		virtual IGraphicFactoryDWrite* AsGraphicFactoryDWrite() = 0;
	};

	class IGraphicFactoryDxgi
	{
	public:
		virtual IDXGIFactoryX* GetDxgiFactory() = 0;
	};

	class IGraphicFactory2d
	{
	public:
		virtual ID2D1FactoryX* Get2dFactory() = 0;
	};

	class IGraphicFactoryDWrite
	{
	public:
		virtual IDWriteFactoryX* GetWriteFactory() = 0;
		virtual IDWriteInMemoryFontFileLoader* GetFontDataLoader() = 0;
	};

	ComPtr<IGraphicFactory> CreateGraphicFactory();
}
