#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphBuffer.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphDeviceChild.h"
#include "Graph/GraphFactory.h"
#include "Graph/Texture/Texture.h"
#include "Graph/Render/Renderer.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTargetSwapChain.h"
#include "Graph/RenderTarget/RenderTargetWindow.h"
#include "Graph/State/GraphContext11.h"
#include "Graph/State/GraphStateCache11.h"
#include "Module/Module.h"
#include "Thread/ThreadDispatch.h"

class __declspec(uuid("dce9ac0d-79da-4456-b441-7bde392df877"))
	GraphDevice11
	: public ff::ComBase
	, public ff::IGraphDevice
	, public ff::IGraphDeviceDxgi
	, public ff::IGraphDevice11
	, public ff::IGraphDeviceInternal
{
public:
	DECLARE_HEADER(GraphDevice11);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init();

	// IGraphDevice
	virtual bool Reset() override;
	virtual bool ResetIfNeeded() override;
	virtual size_t ResetDrawCount() override;
	virtual ff::IGraphDeviceDxgi* AsGraphDeviceDxgi() override;
	virtual ff::IGraphDevice11* AsGraphDevice11() override;
	virtual ff::IGraphDeviceInternal* AsGraphDeviceInternal() override;
	virtual std::unique_ptr<ff::IRenderer> CreateRenderer() override;
	virtual ff::ComPtr<ff::IGraphBuffer> CreateBuffer(ff::GraphBufferType type, size_t size, bool writable, ff::IData* initialData) override;
	virtual ff::ComPtr<ff::ITexture> CreateTexture(ff::StringRef path, ff::TextureFormat format, size_t mips) override;
	virtual ff::ComPtr<ff::ITexture> CreateTexture(DirectX::ScratchImage&& data) override;
	virtual ff::ComPtr<ff::ITexture> CreateTexture(ff::PointInt size, ff::TextureFormat format, size_t mips, size_t count, size_t samples) override;
	virtual ff::ComPtr<ff::ITexture> CreateStagingTexture(ff::PointInt size, ff::TextureFormat format, bool readable, bool writable, size_t mips, size_t count, size_t samples) override;
	virtual ff::ComPtr<ff::IRenderDepth> CreateRenderDepth(ff::PointInt size, size_t samples) override;
	virtual ff::ComPtr<ff::IRenderTarget> CreateRenderTargetTexture(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipLevel) override;
#if METRO_APP
	virtual ff::ComPtr<ff::IRenderTargetWindow> CreateRenderTargetWindow(Windows::UI::Xaml::Window^ hwnd) override;
	virtual ff::ComPtr<ff::IRenderTargetSwapChain> CreateRenderTargetSwapChain(Windows::UI::Xaml::Controls::SwapChainPanel^ panel) override;
#else
	virtual ff::ComPtr<ff::IRenderTargetWindow> CreateRenderTargetWindow(HWND hwnd) override;
#endif
	virtual void AddChild(ff::IGraphDeviceChild* child) override;
	virtual void RemoveChild(ff::IGraphDeviceChild* child) override;

	// IGraphDeviceDxgi
	virtual IDXGIDeviceX* GetDxgi() override;
	virtual IDXGIAdapterX* GetAdapter() override;
	virtual IDXGIFactoryX* GetDxgiFactory() override;

	// IGraphDevice11
	virtual ID3D11DeviceX* Get3d() override;
	virtual ID2D1DeviceX* Get2d() override;
	virtual ID3D11DeviceContextX* GetContext() override;
	virtual ff::GraphContext11& GetStateContext() override;
	virtual ff::GraphStateCache11& GetStateCache() override;

private:
	ff::Mutex _mutex;
	ff::ComPtr<ff::IGraphicFactory> _factory;
	ff::ComPtr<ID3D11DeviceX> _device;
	ff::ComPtr<ID3D11DeviceContextX> _deviceContext;
	ff::ComPtr<ID2D1DeviceX> _device2d;
	ff::ComPtr<IDXGIAdapterX> _dxgiAdapter;
	ff::ComPtr<IDXGIFactoryX> _dxgiFactory;
	ff::ComPtr<IDXGIDeviceX> _dxgiDevice;
	ff::Vector<ff::IGraphDeviceChild*> _children;
	ff::GraphContext11 _stateContext;
	ff::GraphStateCache11 _stateCache;
};

BEGIN_INTERFACES(GraphDevice11)
	HAS_INTERFACE(ff::IGraphDevice)
END_INTERFACES()

ff::ComPtr<ff::IGraphDevice> CreateGraphDevice11(ff::IGraphicFactory* factory)
{
	ff::ComPtr<GraphDevice11, ff::IGraphDevice> graph;
	assertHrRetVal(ff::ComAllocator<GraphDevice11>::CreateInstance(factory, &graph), nullptr);
	assertRetVal(graph->Init(), nullptr);
	return graph.Interface();
}

GraphDevice11::GraphDevice11()
{
}

GraphDevice11::~GraphDevice11()
{
	assert(!_children.Size());

	if (_deviceContext)
	{
		_deviceContext->ClearState();
		_deviceContext->Flush();
	}

	if (_factory)
	{
		_factory->RemoveChild(this);
	}
}

static bool InternalCreateDevice(ID3D11DeviceX** device, ID3D11DeviceContextX** context)
{
	assertRetVal(device && context, false);

	const D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	ff::ComPtr<ID3D11Device> deviceCreate;
	ff::ComPtr<ID3D11DeviceContext> contextCreate;
	bool allowDebug = ff::GetThisModule().IsDebugBuild() && ::IsDebuggerPresent();
	HRESULT hr = E_FAIL;

	for (int isHardware = 1; FAILED(hr) && isHardware >= 0; isHardware--)
	{
		for (int isDebug = allowDebug ? 1 : 0; FAILED(hr) && isDebug >= 0; isDebug--)
		{
			hr = ::D3D11CreateDevice(
				nullptr, // adapter
				isHardware ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_WARP,
				nullptr, // software module
				D3D11_CREATE_DEVICE_BGRA_SUPPORT | (isDebug ? D3D11_CREATE_DEVICE_DEBUG : 0),
				featureLevels,
				_countof(featureLevels),
				D3D11_SDK_VERSION,
				&deviceCreate,
				nullptr, // feature level
				&contextCreate);
		}
	}

	assertHrRetVal(hr, false);

	ff::ComPtr<ID3D11DeviceX> deviceLatest;
	assertRetVal(deviceLatest.QueryFrom(deviceCreate), false);

	ff::ComPtr<ID3D11DeviceContextX> contextLatest;
	assertRetVal(contextLatest.QueryFrom(contextCreate), false);

	*device = deviceLatest.Detach();
	*context = contextLatest.Detach();

	return true;
}

HRESULT GraphDevice11::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_factory.QueryFrom(unkOuter), E_INVALIDARG);
	_factory->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool GraphDevice11::Init()
{
	assertRetVal(!_device && !_deviceContext, false);
	assertRetVal(::InternalCreateDevice(&_device, &_deviceContext), false);
	assertRetVal(_dxgiDevice.QueryFrom(_device), false);

	_dxgiAdapter = ff::GetParentDXGI<IDXGIAdapterX>(_device);
	assertRetVal(_dxgiAdapter, false);

	_dxgiFactory = ff::GetParentDXGI<IDXGIFactoryX>(_dxgiAdapter);
	assertRetVal(_dxgiFactory, false);

	_stateContext.Reset(_deviceContext);
	_stateCache.Reset(_device);

	return true;
}

ff::GraphContext11& GraphDevice11::GetStateContext()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());
	return _stateContext;
}

ff::GraphStateCache11& GraphDevice11::GetStateCache()
{
	return _stateCache;
}

ff::IGraphDeviceDxgi* GraphDevice11::AsGraphDeviceDxgi()
{
	return this;
}

ff::IGraphDevice11* GraphDevice11::AsGraphDevice11()
{
	return this;
}

ff::IGraphDeviceInternal* GraphDevice11::AsGraphDeviceInternal()
{
	return this;
}

std::unique_ptr<ff::IRenderer> CreateRenderer11(ff::IGraphDevice* device);

std::unique_ptr<ff::IRenderer> GraphDevice11::CreateRenderer()
{
	return ::CreateRenderer11(this);
}

bool CreateTexture11(ff::IGraphDevice* device, ff::StringRef path, DXGI_FORMAT format, size_t mips, ff::ITexture** texture);
bool CreateTexture11(ff::IGraphDevice* device, DirectX::ScratchImage&& data, ff::ITexture** texture);
bool CreateTexture11(ff::IGraphDevice* device, ff::PointInt size, DXGI_FORMAT format, size_t mips, size_t count, size_t samples, ff::ITexture** texture);
bool CreateStagingTexture11(ff::IGraphDevice* device, ff::PointInt size, DXGI_FORMAT format, bool readable, bool writable, size_t mips, size_t count, size_t samples, ff::ITexture** texture);
bool CreateGraphBuffer11(ff::IGraphDevice* device, ff::GraphBufferType type, size_t size, bool writable, ff::IData* initialData, ff::IGraphBuffer** buffer);

ff::ComPtr<ff::IGraphBuffer> GraphDevice11::CreateBuffer(ff::GraphBufferType type, size_t size, bool writable, ff::IData* initialData)
{
	ff::ComPtr<ff::IGraphBuffer> obj;
	return ::CreateGraphBuffer11(this, type, size, writable, initialData, &obj) ? obj : nullptr;
}

ff::ComPtr<ff::ITexture> GraphDevice11::CreateTexture(ff::StringRef path, ff::TextureFormat format, size_t mips)
{
	ff::ComPtr<ff::ITexture> obj;
	return ::CreateTexture11(this, path, ff::ConvertTextureFormat(format), mips, &obj) ? obj : nullptr;
}

ff::ComPtr<ff::ITexture> GraphDevice11::CreateTexture(DirectX::ScratchImage&& data)
{
	ff::ComPtr<ff::ITexture> obj;
	return ::CreateTexture11(this, std::move(data), &obj) ? obj : nullptr;
}

ff::ComPtr<ff::ITexture> GraphDevice11::CreateTexture(ff::PointInt size, ff::TextureFormat format, size_t mips, size_t count, size_t samples)
{
	ff::ComPtr<ff::ITexture> obj;
	return ::CreateTexture11(this, size, ff::ConvertTextureFormat(format), mips, count, samples, &obj) ? obj : nullptr;
}

ff::ComPtr<ff::ITexture> GraphDevice11::CreateStagingTexture(ff::PointInt size, ff::TextureFormat format, bool readable, bool writable, size_t mips, size_t count, size_t samples)
{
	ff::ComPtr<ff::ITexture> obj;
	return ::CreateStagingTexture11(this, size, ff::ConvertTextureFormat(format), readable, writable, mips, count, samples, &obj) ? obj : nullptr;
}

bool CreateRenderDepth11(ff::IGraphDevice* pDevice, ff::PointInt size, size_t multiSamples, ff::IRenderDepth** ppDepth);

ff::ComPtr<ff::IRenderDepth> GraphDevice11::CreateRenderDepth(ff::PointInt size, size_t samples)
{
	ff::ComPtr<ff::IRenderDepth> obj;
	return ::CreateRenderDepth11(this, size, samples, &obj) ? obj : nullptr;
}

bool CreateRenderTargetTexture11(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipLevel, ff::IRenderTarget** ppRender);

ff::ComPtr<ff::IRenderTarget> GraphDevice11::CreateRenderTargetTexture(ff::ITexture* texture, size_t arrayStart, size_t arrayCount, size_t mipLevel)
{
	assertRetVal(texture && texture->GetDevice() == this, nullptr);

	ff::ComPtr<ff::IRenderTarget> obj;
	return ::CreateRenderTargetTexture11(texture, arrayStart, arrayCount, mipLevel, &obj) ? obj : nullptr;
}

#if METRO_APP

bool CreateRenderTargetWindow11(ff::IGraphDevice* pDevice, Windows::UI::Xaml::Window^ hwnd, ff::IRenderTargetWindow** ppRender);

ff::ComPtr<ff::IRenderTargetWindow> GraphDevice11::CreateRenderTargetWindow(Windows::UI::Xaml::Window^ hwnd)
{
	ff::ComPtr<ff::IRenderTargetWindow> obj;
	return ::CreateRenderTargetWindow11(this, hwnd, &obj) ? obj : nullptr;
}

#else

bool CreateRenderTargetWindow11(ff::IGraphDevice* pDevice, HWND hwnd, ff::IRenderTargetWindow** ppRender);

ff::ComPtr<ff::IRenderTargetWindow> GraphDevice11::CreateRenderTargetWindow(HWND hwnd)
{
	ff::ComPtr<ff::IRenderTargetWindow> obj;
	return ::CreateRenderTargetWindow11(this, hwnd, &obj) ? obj : nullptr;
}

#endif

#if METRO_APP

bool CreateRenderTargetSwapChain11(ff::IGraphDevice* pDevice, Windows::UI::Xaml::Controls::SwapChainPanel^ panel, ff::IRenderTargetSwapChain** obj);

ff::ComPtr<ff::IRenderTargetSwapChain> GraphDevice11::CreateRenderTargetSwapChain(Windows::UI::Xaml::Controls::SwapChainPanel^ panel)
{
	ff::ComPtr<ff::IRenderTargetSwapChain> obj;
	return ::CreateRenderTargetSwapChain11(this, panel, &obj) ? obj : nullptr;
}

#endif

void GraphDevice11::AddChild(ff::IGraphDeviceChild* child)
{
	ff::LockMutex lock(_mutex);

	assert(child && _children.Find(child) == ff::INVALID_SIZE);
	_children.Push(child);
}

void GraphDevice11::RemoveChild(ff::IGraphDeviceChild* child)
{
	ff::LockMutex lock(_mutex);

	verify(_children.DeleteItem(child));
}

bool GraphDevice11::Reset()
{
	_stateContext.Reset(nullptr);
	_stateCache.Reset(nullptr);

	if (_deviceContext)
	{
		_deviceContext->ClearState();
		_deviceContext->Flush();
		_deviceContext = nullptr;
	}

	_device = nullptr;
	_device2d = nullptr;
	_dxgiAdapter = nullptr;
	_dxgiFactory = nullptr;
	_dxgiDevice = nullptr;

	assertRetVal(Init(), false);

	bool status = true;
	for (auto i = _children.rbegin(); i != _children.rend(); i++)
	{
		if (!(*i)->Reset())
		{
			status = false;
		}
	}

	return status;
}

bool GraphDevice11::ResetIfNeeded()
{
	if (!ff::IsFactoryCurrent(_dxgiFactory) || FAILED(_device->GetDeviceRemovedReason()))
	{
		assertRetVal(Reset(), false);
	}

	return true;
}

size_t GraphDevice11::ResetDrawCount()
{
	return _stateContext.ResetDrawCount();
}

ID3D11DeviceX* GraphDevice11::Get3d()
{
	return _device;
}

ID2D1DeviceX* GraphDevice11::Get2d()
{
	if (!_device2d && _dxgiDevice)
	{
		ff::IGraphicFactory2d* d2d = ff::ProcessGlobals::Get()->GetGraphicFactory()->AsGraphicFactory2d();
		assertRetVal(d2d, nullptr);

		ff::ComPtr<ID2D1FactoryX> factory2d = d2d->Get2dFactory();
		assertRetVal(factory2d, nullptr);

		ff::LockMutex lock(_mutex);

		if (!_device2d && _dxgiDevice)
		{
			assertHrRetVal(factory2d->CreateDevice(_dxgiDevice, &_device2d), nullptr);
		}
	}

	return _device2d;
}

IDXGIDeviceX* GraphDevice11::GetDxgi()
{
	return _dxgiDevice;
}

IDXGIAdapterX* GraphDevice11::GetAdapter()
{
	return _dxgiAdapter;
}

IDXGIFactoryX* GraphDevice11::GetDxgiFactory()
{
	return _dxgiFactory;
}

ID3D11DeviceContextX* GraphDevice11::GetContext()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());
	return _deviceContext;
}
