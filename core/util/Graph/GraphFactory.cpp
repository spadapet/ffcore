#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"

extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

class __declspec(uuid("b38739e9-19e5-4cec-8065-6bb784f7ad39"))
	GraphicFactory
	: public ff::ComBase
	, public ff::IGraphicFactory
	, public ff::IGraphicFactoryDxgi
	, public ff::IGraphicFactory2d
	, public ff::IGraphicFactoryDWrite
{
public:
	DECLARE_HEADER(GraphicFactory);

	// IGraphicFactory
	virtual ff::ComPtr<ff::IGraphDevice> CreateDevice() override;
	virtual size_t GetDeviceCount() const override;
	virtual ff::IGraphDevice* GetDevice(size_t nIndex) const override;

	virtual void AddChild(ff::IGraphDevice* child) override;
	virtual void RemoveChild(ff::IGraphDevice* child) override;

	virtual ff::IGraphicFactoryDxgi* AsGraphicFactoryDxgi() override;
	virtual ff::IGraphicFactory2d* AsGraphicFactory2d() override;
	virtual ff::IGraphicFactoryDWrite* AsGraphicFactoryDWrite() override;

	virtual IDXGIFactoryX* GetDxgiFactory() override;
	virtual ID2D1FactoryX* Get2dFactory() override;
	virtual IDWriteFactoryX* GetWriteFactory() override;
	virtual IDWriteInMemoryFontFileLoader* GetFontDataLoader() override;

private:
	ff::Mutex _mutex;
	ff::ComPtr<IDXGIFactoryX> _factoryDxgi;
	ff::ComPtr<ID2D1FactoryX> _factory2d;
	ff::ComPtr<IDWriteFactoryX> _factoryWrite;
	ff::ComPtr<IDWriteInMemoryFontFileLoader> _fontDataLoader;
	ff::Vector<ff::IGraphDevice*> _devices;
};

BEGIN_INTERFACES(GraphicFactory)
	HAS_INTERFACE(ff::IGraphicFactory)
END_INTERFACES()

ff::ComPtr<ff::IGraphicFactory> ff::CreateGraphicFactory()
{
	ff::ComPtr<ff::IGraphicFactory> obj;
	assertHrRetVal(ff::ComAllocator<GraphicFactory>::CreateInstance(
		nullptr, GUID_NULL, __uuidof(ff::IGraphicFactory), (void**)&obj), false);
	return obj;
}

GraphicFactory::GraphicFactory()
{
}

GraphicFactory::~GraphicFactory()
{
	if (_fontDataLoader)
	{
		_factoryWrite->UnregisterFontFileLoader(_fontDataLoader);
		_fontDataLoader = nullptr;
	}
}

size_t GraphicFactory::GetDeviceCount() const
{
	return _devices.Size();
}

ff::IGraphDevice* GraphicFactory::GetDevice(size_t nIndex) const
{
	assertRetVal(nIndex >= 0 && nIndex < _devices.Size(), nullptr);
	return _devices[nIndex];
}

IDXGIFactoryX* GraphicFactory::GetDxgiFactory()
{
	if (_factoryDxgi && !_factoryDxgi->IsCurrent())
	{
		ff::LockMutex lock(_mutex);

		if (_factoryDxgi && !_factoryDxgi->IsCurrent())
		{
			_factoryDxgi = nullptr;
		}
	}

	if (!_factoryDxgi)
	{
		ff::LockMutex lock(_mutex);

		if (!_factoryDxgi)
		{
			bool allowDebug = ff::GetThisModule().IsDebugBuild() && ::IsDebuggerPresent();
			UINT flags = allowDebug ? DXGI_CREATE_FACTORY_DEBUG : 0;
			assertHrRetVal(::CreateDXGIFactory2(flags, __uuidof(IDXGIFactoryX), (void**)&_factoryDxgi), nullptr);
		}
	}

	return _factoryDxgi;
}

ID2D1FactoryX* GraphicFactory::Get2dFactory()
{
	if (!_factory2d)
	{
		ff::LockMutex lock(_mutex);

		if (!_factory2d)
		{
			D2D1_FACTORY_OPTIONS options;
			options.debugLevel = ff::GetThisModule().IsDebugBuild() ? D2D1_DEBUG_LEVEL_NONE : D2D1_DEBUG_LEVEL_WARNING;
			assertHrRetVal(::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1FactoryX), &options, (void**)&_factory2d), nullptr);
		}
	}

	return _factory2d;
}

IDWriteFactoryX* GraphicFactory::GetWriteFactory()
{
	if (!_factoryWrite)
	{
		ff::LockMutex lock(_mutex);

		if (!_factoryWrite)
		{
			assertHrRetVal(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactoryX), (IUnknown**)&_factoryWrite), nullptr);
		}
	}

	return _factoryWrite;
}

IDWriteInMemoryFontFileLoader* GraphicFactory::GetFontDataLoader()
{
	if (!_fontDataLoader)
	{
		ff::LockMutex lock(_mutex);

		if (!_fontDataLoader)
		{
			ff::ComPtr<IDWriteInMemoryFontFileLoader> fontDataLoader;
			assertRetVal(GetWriteFactory(), nullptr);
			assertHrRetVal(_factoryWrite->CreateInMemoryFontFileLoader(&fontDataLoader), nullptr);
			assertHrRetVal(_factoryWrite->RegisterFontFileLoader(fontDataLoader), nullptr);
			_fontDataLoader = fontDataLoader;
		}
	}

	return _fontDataLoader;
}

ff::ComPtr<ff::IGraphDevice> CreateGraphDevice11(ff::IGraphicFactory* factory);
ff::ComPtr<ff::IGraphDevice> CreateGraphDevice12(ff::IGraphicFactory* factory);

ff::ComPtr<ff::IGraphDevice> GraphicFactory::CreateDevice()
{
	return ::CreateGraphDevice11(this);
}

void GraphicFactory::AddChild(ff::IGraphDevice* child)
{
	ff::LockMutex lock(_mutex);

	assert(child && _devices.Find(child) == ff::INVALID_SIZE);
	_devices.Push(child);
}

void GraphicFactory::RemoveChild(ff::IGraphDevice* child)
{
	ff::LockMutex lock(_mutex);

	verify(_devices.DeleteItem(child));
}

ff::IGraphicFactoryDxgi* GraphicFactory::AsGraphicFactoryDxgi()
{
	return this;
}

ff::IGraphicFactory2d* GraphicFactory::AsGraphicFactory2d()
{
	return this;
}

ff::IGraphicFactoryDWrite* GraphicFactory::AsGraphicFactoryDWrite()
{
	return this;
}
